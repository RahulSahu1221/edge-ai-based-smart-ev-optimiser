#include <Arduino.h>
#include <ArduinoJson.h>

#include "attributes.h"
#include "Network.h"
#include "State.h"
#include "config.h"

// =====================================================================
// Request shared attributes from ThingsBoard
// =====================================================================

void requestSharedAttributes()
{
    if (!mqtt.connected())
    {
        return;
    }

    JsonDocument doc;

    // SRS-defined configuration
    doc["sharedKeys"] =
        "maxStationLoadW,"
        "overloadCurrentA,"
        "peakTariffStart,"
        "peakTariffEnd,"
        "predictionThreshold,"
        "overvoltageThresholdV,"
        "maxSessionDurationMin,"
        "stationBayCount,"
        "neighborBaysOccupied,"
        "neighborPower1,"
        "neighborPower2,"
        "neighborPower3,"
        "neighborDuration1,"
        "neighborDuration2,"
        "neighborDuration3";

    char buffer[512];

    serializeJson(
        doc,
        buffer,
        sizeof(buffer)
    );

    mqtt.publish(
        "v1/devices/me/attributes/request/1",
        buffer
    );

    Serial.println(
        "[ATTR] Shared attributes requested."
    );
}

// =====================================================================
// Apply shared attributes received from ThingsBoard
// =====================================================================

void applySharedAttributes(
    JsonObject attrs)
{
    if (!attrs["maxStationLoadW"].isNull())
    {
        maxStationLoadW =
            attrs["maxStationLoadW"].as<float>();
    }

    if (!attrs["overloadCurrentA"].isNull())
    {
        overloadCurrentA =
            attrs["overloadCurrentA"].as<float>();
    }

    // -------------------------------------------------------------
    // Accept the SRS names:
    // peakTariffStart / peakTariffEnd
    //
    // Also accept the old names from the current project:
    // peakTariffStartHr / peakTariffEndHr
    // -------------------------------------------------------------

    if (!attrs["peakTariffStart"].isNull())
    {
        peakTariffStartHr =
            attrs["peakTariffStart"].as<int>();
    }
    else if (!attrs["peakTariffStartHr"].isNull())
    {
        peakTariffStartHr =
            attrs["peakTariffStartHr"].as<int>();
    }

    if (!attrs["peakTariffEnd"].isNull())
    {
        peakTariffEndHr =
            attrs["peakTariffEnd"].as<int>();
    }
    else if (!attrs["peakTariffEndHr"].isNull())
    {
        peakTariffEndHr =
            attrs["peakTariffEndHr"].as<int>();
    }

    if (!attrs["predictionThreshold"].isNull())
    {
        predictionThreshold =
            attrs["predictionThreshold"].as<float>();
    }

    if (!attrs["overvoltageThresholdV"].isNull())
    {
        overvoltageThresholdV =
            attrs["overvoltageThresholdV"].as<float>();
    }

    if (!attrs["maxSessionDurationMin"].isNull())
    {
        maxSessionDurationMin =
            attrs["maxSessionDurationMin"].as<int>();
    }

    // -------------------------------------------------------------
    // Multi-bay context
    // -------------------------------------------------------------

    if (!attrs["stationBayCount"].isNull())
    {
        stationBayCount =
            attrs["stationBayCount"].as<int>();

        if (stationBayCount < 1)
        {
            stationBayCount = 1;
        }
    }

    if (!attrs["neighborBaysOccupied"].isNull())
    {
        neighborBaysOccupied =
            attrs["neighborBaysOccupied"].as<int>();
    }

    if (!attrs["neighborPower1"].isNull())
    {
        neighborPower[0] =
            attrs["neighborPower1"].as<float>();
    }

    if (!attrs["neighborPower2"].isNull())
    {
        neighborPower[1] =
            attrs["neighborPower2"].as<float>();
    }

    if (!attrs["neighborPower3"].isNull())
    {
        neighborPower[2] =
            attrs["neighborPower3"].as<float>();
    }

    if (!attrs["neighborDuration1"].isNull())
    {
        neighborDurationMin[0] =
            attrs["neighborDuration1"].as<int>();
    }

    if (!attrs["neighborDuration2"].isNull())
    {
        neighborDurationMin[1] =
            attrs["neighborDuration2"].as<int>();
    }

    if (!attrs["neighborDuration3"].isNull())
    {
        neighborDurationMin[2] =
            attrs["neighborDuration3"].as<int>();
    }

    Serial.println(
        "[ATTR] Shared attributes updated."
    );

    Serial.print(
        "  maxStationLoadW = "
    );

    Serial.println(
        maxStationLoadW
    );

    Serial.print(
        "  overloadCurrentA = "
    );

    Serial.println(
        overloadCurrentA
    );

    Serial.print(
        "  peakTariff = "
    );

    Serial.print(
        peakTariffStartHr
    );

    Serial.print("-");

    Serial.println(
        peakTariffEndHr
    );

    Serial.print(
        "  predictionThreshold = "
    );

    Serial.println(
        predictionThreshold
    );

    Serial.print(
        "  neighborBaysOccupied = "
    );

    Serial.println(
        neighborBaysOccupied
    );
}

// =====================================================================
// Publish client-side attributes
//
// SRS 7.3 requires firmware version and IP address as client-side
// attributes.
// =====================================================================

void publishClientAttributes()
{
    if (!mqtt.connected())
    {
        return;
    }

    JsonDocument doc;

    doc["bayId"] =
        BAY_ID;

    doc["firmwareVersion"] =
        FIRMWARE_VERSION;

    doc["ipAddress"] =
        WiFi.localIP().toString();

    char buffer[256];

    serializeJson(
        doc,
        buffer,
        sizeof(buffer)
    );

    mqtt.publish(
        "v1/devices/me/attributes",
        buffer
    );

    Serial.print(
        "[ATTR >>] "
    );

    Serial.println(
        buffer
    );
}
