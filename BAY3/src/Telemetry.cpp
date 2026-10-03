#include <Arduino.h>
#include <ArduinoJson.h>

#include "Telemetry.h"
#include "Network.h"
#include "State.h"
#include "config.h"

// =====================================================================
// Publish telemetry to ThingsBoard
// =====================================================================

void publishTelemetry()
{
    if (!mqtt.connected())
    {
        return;
    }

    // -------------------------------------------------------------
    // Recalculate station total from local + neighbors.
    // -------------------------------------------------------------

    stationTotalPowerW =
        power;

    for (int i = 0;
         i < MAX_NEIGHBOR_BAYS;
         i++)
    {
        stationTotalPowerW +=
            neighborPower[i];
    }

    // -------------------------------------------------------------
    // JSON document
    // -------------------------------------------------------------

    JsonDocument doc;

    // Basic identity
    doc["bayId"] =
        BAY_ID;

    // Sensor data
    doc["voltage"] =
        round(voltage * 10.0f) / 10.0f;

    doc["current"] =
        round(current * 10.0f) / 10.0f;

    doc["power"] =
        round(power * 10.0f) / 10.0f;

    doc["temperature"] =
        round(temperature * 10.0f) / 10.0f;

    // Energy
    doc["energyWh"] =
        round(energyWh * 100.0f) / 100.0f;

    // Bay state
    doc["bayStatus"] =
        bayStatus;

    // Edge AI
    doc["predictedArrivalProb"] =
        round(predictedArrivalProb * 1000.0f)
        / 1000.0f;

    doc["predictedDurationMin"] =
        predictedDurationMin;

    // Optimization
    doc["loadDecision"] =
        loadDecision;

    doc["optimizationReason"] =
        optimizationReason;

    doc["throttleLevel"] =
        throttleLevel;

    // Safety
    doc["overloadActive"] =
        overloadActive;

    doc["overvoltageActive"] =
        overvoltageActive;

    doc["sensorFault"] =
        sensorFault;

    doc["stuckSession"] =
        stuckSession;

    // Manual operation
    doc["manualOverrideActive"] =
        manualOverrideActive;

    // Station-wide context
    doc["stationTotalPowerW"] =
        round(stationTotalPowerW * 10.0f)
        / 10.0f;

    doc["neighborBaysOccupied"] =
        neighborBaysOccupied;

    doc["stationBayCount"] =
        stationBayCount;

    // Session
    float sessionElapsedMin = 0.0f;

    if (bayStatus == "CHARGING" &&
        sessionStartMs > 0)
    {
        sessionElapsedMin =
            (millis() - sessionStartMs)
            / 60000.0f;
    }

    doc["sessionElapsedMin"] =
        round(sessionElapsedMin * 10.0f)
        / 10.0f;

    // -------------------------------------------------------------
    // Serialize
    // -------------------------------------------------------------

    char buffer[1024];

    size_t length =
        serializeJson(
            doc,
            buffer,
            sizeof(buffer)
        );

    if (length == 0)
    {
        Serial.println(
            "[MQTT] Telemetry serialization failed."
        );

        return;
    }

    // -------------------------------------------------------------
    // Publish
    // -------------------------------------------------------------

    bool success =
        mqtt.publish(
            "v1/devices/me/telemetry",
            buffer
        );

    if (success)
    {
        Serial.print(
            "[MQTT >>] "
        );

        Serial.println(buffer);
    }
    else
    {
        Serial.println(
            "[MQTT] Telemetry publish failed."
        );
    }
}
