#include <Arduino.h>
#include "WiFi.h"

#include "State.h"
#include "config.h"
#include "Peripherals.h"
#include "Network.h"
#include "Telemetry.h"
#include "model.h"
#include "edge_ai.h"
#include "optimization.h"
#include "attributes.h"
#include "rpc.h"

// -----------------------------------------------------------------------------
// Timing
//
// Each cycle now uses its OWN interval from config.h, matching the SRS:
//   - Sensor sampling  (FR-1)      default 5 s      (SENSOR_INTERVAL_MS)
//   - Telemetry publish(FR-7)      default 5 s      (TELEMETRY_INTERVAL_MS)
//   - Local optimization (8.7)     default 30 s      (OPTIMIZATION_INTERVAL_MS)
//   - Edge AI inference (8.6.1)    default 15 min   (AI_INTERVAL_MS)
//
// Previously all four ran on one hardcoded 5-second block.
// -----------------------------------------------------------------------------

unsigned long lastSensorMs = 0;
unsigned long lastTelemetryMs = 0;
unsigned long lastOptimizationMs = 0;
unsigned long lastAiMs = 0;

unsigned long lastWifiAttemptMs = 0;
unsigned long lastMqttAttemptMs = 0;

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------

void setup()
{
    Serial.begin(115200);

    delay(500);

    Serial.println();
    Serial.println("====================================");
    Serial.print(" ");
    Serial.print(BAY_ID);
    Serial.println(" EV CHARGING OPTIMIZER");
    Serial.println("====================================");

    // -------------------------------------------------------------------------
    // Sensors
    // -------------------------------------------------------------------------

    dht.begin();

    // -------------------------------------------------------------------------
    // NTP time
    // -------------------------------------------------------------------------

    configTime(
        0,
        0,
        "pool.ntp.org",
        "time.nist.gov"
    );

    // -------------------------------------------------------------------------
    // GPIO
    // -------------------------------------------------------------------------

    pinMode(BTN_PLUGIN, INPUT_PULLUP);
    pinMode(BTN_PLUGOUT, INPUT_PULLUP);

    pinMode(RELAY_PIN, OUTPUT);

    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(LED_RED, OUTPUT);

    // -------------------------------------------------------------------------
    // Safe initial relay state
    // -------------------------------------------------------------------------

    digitalWrite(RELAY_PIN, LOW);

    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, LOW);

    bayStatus = "FREE";
    throttleLevel = 0;
    loadDecision = "ALLOW";
    manualOverrideActive = false;

    // -------------------------------------------------------------------------
    // Wi-Fi
    // -------------------------------------------------------------------------

    connectWiFi();

    // -------------------------------------------------------------------------
    // MQTT
    // -------------------------------------------------------------------------

    mqtt.setServer(
        MQTT_SERVER,
        MQTT_PORT
    );

    mqtt.setCallback(mqttCallback);

    mqtt.setBufferSize(512);

    connectMQTT();

    // -------------------------------------------------------------------------
    // Prime an initial sensor reading + AI prediction.
    //
    // AI_INTERVAL_MS defaults to 15 minutes, so without this the bay would
    // otherwise run with a zeroed prediction for its first 15 minutes.
    // -------------------------------------------------------------------------

    sample_sensor();
    updateEnergy();
    runEdgeAIInference();

    unsigned long primedMs = millis();

    lastSensorMs = primedMs;
    lastAiMs = primedMs;

    Serial.println("====================================");
    Serial.print(" ");
    Serial.print(BAY_ID);
    Serial.println(" READY");
    Serial.println("====================================");
}

// -----------------------------------------------------------------------------
// LOOP
// -----------------------------------------------------------------------------

void loop()
{
    unsigned long nowMs = millis();

    // -------------------------------------------------------------------------
    // Wi-Fi / MQTT connection maintenance (NFR-Reliability)
    //
    // Previously, only connectMQTT() was retried in loop() - if Wi-Fi itself
    // dropped, connectWiFi() was never called again after setup(), so the
    // device could never recover from a Wi-Fi outage on its own. Both
    // attempts are rate-limited so a dropped connection doesn't block loop().
    // -------------------------------------------------------------------------

    if (WiFi.status() != WL_CONNECTED)
    {
        if (nowMs - lastWifiAttemptMs >= WIFI_RECONNECT_INTERVAL_MS)
        {
            lastWifiAttemptMs = nowMs;
            connectWiFi();
        }
    }
    else if (!mqtt.connected())
    {
        if (nowMs - lastMqttAttemptMs >= MQTT_RECONNECT_INTERVAL_MS)
        {
            lastMqttAttemptMs = nowMs;
            connectMQTT();
        }
    }

    mqtt.loop();

    // -------------------------------------------------------------------------
    // EV plug-in / plug-out buttons
    //
    // Checked every loop iteration (not gated behind a timer) so button
    // presses stay responsive.
    // -------------------------------------------------------------------------

    plug_status();

    // -------------------------------------------------------------------------
    // Sensor sampling (SRS FR-1): default every 5 s
    // -------------------------------------------------------------------------

    if (nowMs - lastSensorMs >= SENSOR_INTERVAL_MS)
    {
        lastSensorMs = nowMs;

        sample_sensor();
        updateEnergy();
    }

    // -------------------------------------------------------------------------
    // Edge AI inference (SRS 8.6.1): default every 15 min
    // -------------------------------------------------------------------------

    if (nowMs - lastAiMs >= AI_INTERVAL_MS)
    {
        lastAiMs = nowMs;

        runEdgeAIInference();
    }

    // -------------------------------------------------------------------------
    // Local optimization (SRS 8.7): default every 30 s
    // -------------------------------------------------------------------------

    if (nowMs - lastOptimizationMs >= OPTIMIZATION_INTERVAL_MS)
    {
        lastOptimizationMs = nowMs;

        if (!manualOverrideActive)
        {
            runOptimization();
        }
    }

    // -------------------------------------------------------------------------
    // Relay + LEDs
    //
    // Applied on EVERY loop iteration, not gated behind a timer.
    // Intermediate throttle levels are a time-proportional duty cycle over
    // a 2 s period - previously this was only evaluated once every 5 s,
    // so a 50% throttle never actually pulsed, it just froze the relay at
    // whatever phase happened to be true at that single 5 s instant.
    // -------------------------------------------------------------------------

    if (manualOverrideActive)
    {
        // Manual RPC control remains authoritative.
        if (throttleLevel >= 100)
        {
            digitalWrite(RELAY_PIN, HIGH);
        }
        else if (throttleLevel <= 0)
        {
            digitalWrite(RELAY_PIN, LOW);
        }
        else
        {
            unsigned long period = 2000;
            unsigned long phase = millis() % period;
            unsigned long onTime =
                (period * throttleLevel) / 100;

            digitalWrite(
                RELAY_PIN,
                phase < onTime ? HIGH : LOW
            );
        }
    }
    else
    {
        applyRelayDutyCycle();
    }

    updateLeds();

    // -------------------------------------------------------------------------
    // Telemetry (SRS FR-7): default every 5 s
    // -------------------------------------------------------------------------

    if (nowMs - lastTelemetryMs >= TELEMETRY_INTERVAL_MS)
    {
        lastTelemetryMs = nowMs;

        publishTelemetry();
    }
}
