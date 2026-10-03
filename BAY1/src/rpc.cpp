#include <Arduino.h>
#include <ArduinoJson.h>

#include "rpc.h"
#include "Network.h"
#include "State.h"
#include "config.h"

// -----------------------------------------------------------------------------
// Robust ThingsBoard RPC handler
//
// Supports:
//   {"method":"setRelayState","params":true}
//   {"method":"setRelayState","params":false}
//
// Also supports:
//   {"method":"setRelayState","params":{"state":true}}
//   {"method":"setRelayState","params":{"enabled":true}}
//   {"method":"setRelayState","params":{"relayState":true}}
//
// And throttle:
//   {"method":"setThrottle","params":60}
//   {"method":"setThrottle","params":{"level":60}}
// -----------------------------------------------------------------------------

void handleRpc(String requestId, char* payload)
{
    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
        Serial.print("[RPC ERROR] Invalid JSON: ");
        Serial.println(error.c_str());
        return;
    }

    String method = doc["method"] | "";

    Serial.print("[RPC] Method: ");
    Serial.println(method);

    JsonVariant params = doc["params"];

    JsonDocument response;

    // =========================================================================
    // SET RELAY STATE
    // =========================================================================
    if (method == "setRelayState")
    {
        bool relayState = false;
        bool validCommand = false;

        // ---------------------------------------------------------------------
        // Case 1:
        // params = true / false
        // ---------------------------------------------------------------------
        if (params.is<bool>())
        {
            relayState = params.as<bool>();
            validCommand = true;
        }

        // ---------------------------------------------------------------------
        // Case 2:
        // params = {"state": true}
        // ---------------------------------------------------------------------
        else if (params["state"].is<bool>())
        {
            relayState = params["state"].as<bool>();
            validCommand = true;
        }

        // ---------------------------------------------------------------------
        // Case 3:
        // params = {"enabled": true}
        // ---------------------------------------------------------------------
        else if (params["enabled"].is<bool>())
        {
            relayState = params["enabled"].as<bool>();
            validCommand = true;
        }

        // ---------------------------------------------------------------------
        // Case 4:
        // params = {"relayState": true}
        // ---------------------------------------------------------------------
        else if (params["relayState"].is<bool>())
        {
            relayState = params["relayState"].as<bool>();
            validCommand = true;
        }

        // ---------------------------------------------------------------------
        // Reject invalid commands
        // ---------------------------------------------------------------------
        if (!validCommand)
        {
            response["success"] = false;
            response["error"] = "Invalid relay parameter";

            Serial.println("[RPC ERROR] Invalid setRelayState parameters.");
        }
        else
        {
            // -------------------------------------------------------------
            // Physically operate the relay
            // -------------------------------------------------------------
            digitalWrite(
                RELAY_PIN,
                relayState ? HIGH : LOW
            );

            // -------------------------------------------------------------
            // Update internal system state
            // -------------------------------------------------------------
            manualOverrideActive = true;

            if (relayState)
            {
                bayStatus = "CHARGING";
                throttleLevel = 100;
                loadDecision = "MANUAL_ON";

                if (sessionStartMs == 0)
                {
                    sessionStartMs = millis();
                }
            }
            else
            {
                bayStatus = "FREE";
                throttleLevel = 0;
                loadDecision = "MANUAL_OFF";

                sessionStartMs = 0;
            }

            response["success"] = true;
            response["relayState"] = relayState;
            response["bayStatus"] = bayStatus;
            response["manualOverride"] = true;
            response["throttleLevel"] = throttleLevel;

            Serial.print("[RPC] Relay -> ");
            Serial.println(relayState ? "ON" : "OFF");
        }
    }

    // =========================================================================
    // SET THROTTLE
    // =========================================================================
    else if (method == "setThrottle")
    {
        int level = 100;
        bool validCommand = false;

        // ---------------------------------------------------------------------
        // Case 1:
        // params = 60
        // ---------------------------------------------------------------------
        if (params.is<int>() || params.is<float>())
        {
            level = params.as<int>();
            validCommand = true;
        }

        // ---------------------------------------------------------------------
        // Case 2:
        // params = {"level":60}
        // ---------------------------------------------------------------------
        else if (params["level"].is<int>() || params["level"].is<float>())
        {
            level = params["level"].as<int>();
            validCommand = true;
        }

        level = constrain(level, 0, 100);

        if (!validCommand)
        {
            response["success"] = false;
            response["error"] = "Invalid throttle parameter";

            Serial.println("[RPC ERROR] Invalid setThrottle parameters.");
        }
        else
        {
            manualOverrideActive = true;
            throttleLevel = level;
            loadDecision = "MANUAL_THROTTLE";

            // -------------------------------------------------------------
            // Apply throttle immediately.
            //
            // 100% = relay ON
            // 0%   = relay OFF
            // Intermediate values are handled by duty cycling.
            // -------------------------------------------------------------
            if (bayStatus != "CHARGING")
            {
                bayStatus = "CHARGING";
                sessionStartMs = millis();
            }

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

            response["success"] = true;
            response["throttleLevel"] = throttleLevel;
            response["manualOverride"] = true;

            Serial.print("[RPC] Throttle -> ");
            Serial.print(throttleLevel);
            Serial.println("%");
        }
    }

    // =========================================================================
    // GET STATUS
    //
    // SRS Table 11 requires an immediate status report on demand, with
    // no params. Reuses the same fields published in telemetry.
    // =========================================================================
    else if (method == "getStatus")
    {
        response["success"] = true;

        response["bayId"] = BAY_ID;
        response["bayStatus"] = bayStatus;

        response["voltage"] = voltage;
        response["current"] = current;
        response["power"] = power;
        response["energyWh"] = energyWh;
        response["temperature"] = temperature;

        response["predictedArrivalProb"] = predictedArrivalProb;
        response["predictedDurationMin"] = predictedDurationMin;

        response["loadDecision"] = loadDecision;
        response["optimizationReason"] = optimizationReason;
        response["throttleLevel"] = throttleLevel;

        response["manualOverrideActive"] = manualOverrideActive;

        Serial.println("[RPC] Status requested.");
    }

    // =========================================================================
    // CLEAR MANUAL OVERRIDE
    // =========================================================================
    else if (method == "clearManualOverride")
    {
        manualOverrideActive = false;

        response["success"] = true;
        response["manualOverride"] = false;

        Serial.println("[RPC] Manual override cleared.");
    }

    // =========================================================================
    // UNKNOWN RPC
    // =========================================================================
    else
    {
        response["success"] = false;
        response["error"] = "Unknown RPC method";

        Serial.print("[RPC ERROR] Unknown method: ");
        Serial.println(method);
    }

    // =========================================================================
    // Send RPC response back to ThingsBoard
    // =========================================================================
    char buffer[512];

    serializeJson(response, buffer);

    String responseTopic =
        "v1/devices/me/rpc/response/" + requestId;

    mqtt.publish(
        responseTopic.c_str(),
        buffer
    );

    Serial.print("[RPC >>] ");
    Serial.println(buffer);
}