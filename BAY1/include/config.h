#ifndef CONFIG_H
#define CONFIG_H

// =====================================================================
// EV CHARGING STATION OPTIMIZER - CONFIGURATION
// =====================================================================

// ---------------------------------------------------------------------
// Hardware / Wokwi pin configuration
// ---------------------------------------------------------------------

#define VOLTAGE_PIN     34
#define CURRENT_PIN     35
#define DHT_PIN         15

#define RELAY_PIN       26

#define BTN_PLUGIN      32
#define BTN_PLUGOUT     33

#define LED_GREEN       18
#define LED_YELLOW      19
#define LED_RED         21

#define DHT_TYPE        DHT22

// ---------------------------------------------------------------------
// Timing configuration
// ---------------------------------------------------------------------

// SRS FR-1: default sensor sampling interval = 5 seconds
#define SENSOR_INTERVAL_MS             5000UL

// SRS 8.7: optimization cycle = approximately 30 seconds
#define OPTIMIZATION_INTERVAL_MS      30000UL

// SRS 8.6.1: Edge AI evaluation cycle = approximately 15 minutes
#define AI_INTERVAL_MS                900000UL

// Telemetry should be frequent enough for the dashboard and T-01.
// 5 seconds keeps telemetry aligned with the sensor cycle.
#define TELEMETRY_INTERVAL_MS          5000UL

// MQTT reconnect retry interval
#define MQTT_RECONNECT_INTERVAL_MS     5000UL

// Wi-Fi reconnect retry interval
#define WIFI_RECONNECT_INTERVAL_MS    10000UL

// ---------------------------------------------------------------------
// Safety defaults
// ---------------------------------------------------------------------

#define DEFAULT_MAX_STATION_LOAD_W     6000.0f
#define DEFAULT_OVERLOAD_CURRENT_A       16.0f
#define DEFAULT_PREDICTION_THRESHOLD      0.50f

#define DEFAULT_PEAK_TARIFF_START_HR     18
#define DEFAULT_PEAK_TARIFF_END_HR       21

#define DEFAULT_OVERVOLTAGE_THRESHOLD_V  250.0f
#define DEFAULT_MAX_SESSION_DURATION_MIN 180

// ---------------------------------------------------------------------
// Multi-bay configuration
// ---------------------------------------------------------------------

// Maximum number of other bays this firmware can receive as context.
#define MAX_NEIGHBOR_BAYS 3

// ---------------------------------------------------------------------
// Firmware information
// ---------------------------------------------------------------------

#define FIRMWARE_VERSION "1.0.0"

// ---------------------------------------------------------------------
// Wi-Fi
// ---------------------------------------------------------------------

extern const char* WIFI_SSID;
extern const char* WIFI_PASS;

// ---------------------------------------------------------------------
// ThingsBoard / MQTT
// ---------------------------------------------------------------------

extern const char* MQTT_SERVER;
extern const int MQTT_PORT;

extern const char* TB_TOKEN;
extern const char* BAY_ID;

#endif
