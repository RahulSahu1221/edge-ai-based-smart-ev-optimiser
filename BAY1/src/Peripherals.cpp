#include <Arduino.h>
#include <DHT.h>

#include "State.h"
#include "Peripherals.h"
#include "config.h"

DHT dht(DHT_PIN, DHT_TYPE);

// -----------------------------------------------------------------------------
// Floating-point map
// -----------------------------------------------------------------------------

float mapFloat(
    long x,
    long inMin,
    long inMax,
    float outMin,
    float outMax
)
{
    return
        (x - inMin) *
        (outMax - outMin) /
        (float)(inMax - inMin)
        + outMin;
}

// -----------------------------------------------------------------------------
// SENSOR SAMPLING
// -----------------------------------------------------------------------------

void sample_sensor(void)
{
    int raw_current =
        analogRead(CURRENT_PIN);

    int raw_voltage =
        analogRead(VOLTAGE_PIN);

    // -------------------------------------------------------------------------
    // Voltage
    // -------------------------------------------------------------------------

    voltage =
        mapFloat(
            raw_voltage,
            0,
            4095,
            0,
            250
        );

    // -------------------------------------------------------------------------
    // Current
    // -------------------------------------------------------------------------

    if (bayStatus == "CHARGING")
    {
        current =
            mapFloat(
                raw_current,
                0,
                4095,
                0,
                32
            );
    }
    else
    {
        current = 0.0f;
    }

    // -------------------------------------------------------------------------
    // Power
    // -------------------------------------------------------------------------

    power = voltage * current;

    // -------------------------------------------------------------------------
    // Temperature
    // -------------------------------------------------------------------------

    float t = dht.readTemperature();

    if (!isnan(t))
    {
        temperature = t;

        // A valid reading clears any previously-latched fault.
        sensorFault = false;
    }
    else
    {
        // SRS FR-10 / Table 14: sensor disconnection or fault must be
        // detectable. This flag was previously declared and checked in
        // optimization.cpp but never actually set - so the safety branch
        // was unreachable. It is now set here on a real read failure.
        sensorFault = true;

        Serial.println(
            "[SENSOR] Failed to read DHT22"
        );
    }
}

// -----------------------------------------------------------------------------
// RECENT CURRENT AVERAGE
// -----------------------------------------------------------------------------

float recentAvgCurrent(void)
{
    // The existing implementation was averaging the same
    // current value five times. Keep the interface but return
    // the latest valid current measurement.

    return current;
}

// -----------------------------------------------------------------------------
// BUTTON EDGE DETECTION
// -----------------------------------------------------------------------------

bool plugin_flag_once = true;
bool plugout_flag_once = true;

// -----------------------------------------------------------------------------
// PLUG-IN / PLUG-OUT
// -----------------------------------------------------------------------------

void plug_status(void)
{
    // =========================================================================
    // PLUG-IN
    // =========================================================================

    bool pluginReading =
        digitalRead(BTN_PLUGIN);

    if (
        pluginReading == LOW &&
        plugin_flag_once
    )
    {
        plugin_flag_once = false;

        // Only allow automatic plug-in when
        // the operator has not forced the bay OFF.
        if (!manualOverrideActive &&
            bayStatus == "FREE")
        {
            sessionStartMs = millis();

            bayStatus = "CHARGING";

            throttleLevel = 100;

            loadDecision = "ALLOW";

            digitalWrite(
                RELAY_PIN,
                HIGH
            );

            Serial.println(
                "[BUTTON] EV plugged in -> CHARGING -> RELAY ON"
            );
        }
    }

    if (pluginReading == HIGH)
    {
        plugin_flag_once = true;
    }

    // =========================================================================
    // PLUG-OUT
    // =========================================================================

    bool plugoutReading =
        digitalRead(BTN_PLUGOUT);

    if (
        plugoutReading == LOW &&
        plugout_flag_once
    )
    {
        plugout_flag_once = false;

        if (bayStatus == "CHARGING")
        {
            bayStatus = "FREE";

            throttleLevel = 0;

            loadDecision = "ALLOW";

            // -------------------------------------------------------------
            // IMPORTANT:
            // Physically switch the charger OFF.
            // -------------------------------------------------------------

            digitalWrite(
                RELAY_PIN,
                LOW
            );

            Serial.println(
                "[BUTTON] EV unplugged -> FREE -> RELAY OFF"
            );

            // Start next session from zero.
            sessionStartMs = 0;
        }
    }

    if (plugoutReading == HIGH)
    {
        plugout_flag_once = true;
    }
}

// -----------------------------------------------------------------------------
// LED STATUS
// -----------------------------------------------------------------------------

void update_led_status(void)
{
    if (bayStatus == "FREE")
    {
        digitalWrite(
            LED_GREEN,
            HIGH
        );

        digitalWrite(
            LED_YELLOW,
            LOW
        );

        digitalWrite(
            LED_RED,
            LOW
        );
    }
    else if (bayStatus == "CHARGING")
    {
        digitalWrite(
            LED_GREEN,
            LOW
        );

        digitalWrite(
            LED_YELLOW,
            HIGH
        );
    }
    else
    {
        digitalWrite(
            LED_GREEN,
            LOW
        );

        digitalWrite(
            LED_YELLOW,
            LOW
        );
    }
}