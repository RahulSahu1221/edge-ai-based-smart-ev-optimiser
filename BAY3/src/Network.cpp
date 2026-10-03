#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "Network.h"
#include "config.h"
#include "attributes.h"
#include "rpc.h"

// =====================================================================
// Network objects
// =====================================================================

WiFiClient espClient;

PubSubClient mqtt(espClient);

// =====================================================================
// Wi-Fi connection
// =====================================================================

void connectWiFi()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }

    Serial.print(
        "[WiFi] Connecting"
    );

    WiFi.begin(
        WIFI_SSID,
        WIFI_PASS
    );

    unsigned long start =
        millis();

    while (
        WiFi.status() != WL_CONNECTED &&
        millis() - start < 15000UL
    )
    {
        delay(300);

        Serial.print(".");
    }

    Serial.println();

    if (WiFi.status() ==
        WL_CONNECTED)
    {
        Serial.print(
            "[WiFi] Connected. IP = "
        );

        Serial.println(
            WiFi.localIP()
        );
    }
    else
    {
        Serial.println(
            "[WiFi] Connection failed. "
            "Will retry automatically."
        );
    }
}

// =====================================================================
// MQTT connection
// =====================================================================

void connectMQTT()
{
    if (WiFi.status() !=
        WL_CONNECTED)
    {
        return;
    }

    if (mqtt.connected())
    {
        return;
    }

    Serial.print(
        "[MQTT] Connecting to ThingsBoard..."
    );

    bool connected =
        mqtt.connect(
            BAY_ID,
            TB_TOKEN,
            nullptr
        );

    if (connected)
    {
        Serial.println(
            " connected."
        );

        // ---------------------------------------------------------
        // Shared attribute updates
        // ---------------------------------------------------------

        mqtt.subscribe(
            "v1/devices/me/attributes"
        );

        // ---------------------------------------------------------
        // Shared attribute request responses
        // ---------------------------------------------------------

        mqtt.subscribe(
            "v1/devices/me/attributes/response/+"
        );

        // ---------------------------------------------------------
        // RPC requests
        // ---------------------------------------------------------

        mqtt.subscribe(
            "v1/devices/me/rpc/request/+"
        );

        // ---------------------------------------------------------
        // Publish client-side information
        // ---------------------------------------------------------

        publishClientAttributes();

        // ---------------------------------------------------------
        // Request current shared configuration
        // ---------------------------------------------------------

        requestSharedAttributes();
    }
    else
    {
        Serial.print(
            " failed. MQTT state = "
        );

        Serial.println(
            mqtt.state()
        );
    }
}

// =====================================================================
// MQTT callback
// =====================================================================

void mqttCallback(
    char* topic,
    byte* payload,
    unsigned int length)
{
    // -------------------------------------------------------------
    // Copy payload safely into a local buffer.
    // -------------------------------------------------------------

    char buffer[1024];

    unsigned int copyLength =
        length < sizeof(buffer) - 1
        ? length
        : sizeof(buffer) - 1;

    memcpy(
        buffer,
        payload,
        copyLength
    );

    buffer[copyLength] =
        '\0';

    String topicString =
        String(topic);

    Serial.print(
        "[MQTT <<] "
    );

    Serial.print(
        topicString
    );

    Serial.print(
        " -> "
    );

    Serial.println(
        buffer
    );

    // -------------------------------------------------------------
    // RPC
    // -------------------------------------------------------------

    if (
        topicString.startsWith(
            "v1/devices/me/rpc/request/"
        )
    )
    {
        String requestId =
            topicString.substring(
                topicString.lastIndexOf('/') + 1
            );

        handleRpc(
            requestId,
            buffer
        );

        return;
    }

    // -------------------------------------------------------------
    // Attributes
    // -------------------------------------------------------------

    JsonDocument doc;

    DeserializationError error =
        deserializeJson(
            doc,
            buffer
        );

    if (error)
    {
        Serial.print(
            "[MQTT] JSON parse error: "
        );

        Serial.println(
            error.c_str()
        );

        return;
    }

    JsonObject attrs;

    // Attribute request response:
    //
    // {
    //   "shared": {
    //      ...
    //   }
    // }

    if (
        doc["shared"].is<JsonObject>()
    )
    {
        attrs =
            doc["shared"].as<JsonObject>();
    }
    else
    {
        attrs =
            doc.as<JsonObject>();
    }

    applySharedAttributes(
        attrs
    );
}
