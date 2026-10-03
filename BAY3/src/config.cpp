#include "config.h"
#include "secrets.h"

// =====================================================================
// Wi-Fi
// =====================================================================

const char* WIFI_SSID = SECRET_WIFI_SSID;
const char* WIFI_PASS = SECRET_WIFI_PASS;

// =====================================================================
// ThingsBoard
// =====================================================================

// ThingsBoard Cloud MQTT server
const char* MQTT_SERVER = "mqtt.thingsboard.cloud";

// 1883 is used for the Wokwi prototype.
// For a production deployment, use the TLS configuration documented
// by ThingsBoard instead of plaintext MQTT.
const int MQTT_PORT = 1883;

// =====================================================================
// Device
// =====================================================================

// Per-device identity and access token now ACTUALLY come from secrets.h
// (previously this still hardcoded the real token here directly below
// a comment claiming it came from secrets.h - secrets.h was unused).
// secrets.h is NOT committed to git - see secrets.example.h and
// .gitignore. This is the ONLY file that should differ between the
// Bay1 / Bay2 / Bay3 firmware, and only via each bay's own secrets.h.
const char* TB_TOKEN = SECRET_TB_TOKEN;
const char* BAY_ID = SECRET_BAY_ID;
