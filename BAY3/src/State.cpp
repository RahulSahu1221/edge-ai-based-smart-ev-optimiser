#include <Arduino.h>
#include "State.h"

// =====================================================================
// Live bay state
// =====================================================================

String bayStatus = "FREE";

float voltage = 0.0f;
float current = 0.0f;
float power = 0.0f;
float energyWh = 0.0f;
float temperature = 0.0f;

unsigned long sessionStartMs = 0;

// =====================================================================
// Edge AI
// =====================================================================

float predictedArrivalProb = 0.0f;
int predictedDurationMin = 0;

int lastHourOfDay = 12;
int lastDayOfWeek = 0;

// =====================================================================
// Optimization
// =====================================================================

String loadDecision = "ALLOW";
String optimizationReason = "NORMAL";

int throttleLevel = 100;

// =====================================================================
// Shared configuration defaults
// =====================================================================

float predictionThreshold = DEFAULT_PREDICTION_THRESHOLD;

int peakTariffStartHr = DEFAULT_PEAK_TARIFF_START_HR;
int peakTariffEndHr = DEFAULT_PEAK_TARIFF_END_HR;

float overloadCurrentA = DEFAULT_OVERLOAD_CURRENT_A;
float maxStationLoadW = DEFAULT_MAX_STATION_LOAD_W;

float overvoltageThresholdV =
    DEFAULT_OVERVOLTAGE_THRESHOLD_V;

int maxSessionDurationMin =
    DEFAULT_MAX_SESSION_DURATION_MIN;

// =====================================================================
// Safety
// =====================================================================

bool overloadActive = false;
bool overvoltageActive = false;
bool sensorFault = false;
bool stuckSession = false;

// =====================================================================
// Multi-bay
// =====================================================================

float stationTotalPowerW = 0.0f;

int neighborBaysOccupied = 0;

float neighborPower[MAX_NEIGHBOR_BAYS] =
{
    0.0f,
    0.0f,
    0.0f
};

int neighborDurationMin[MAX_NEIGHBOR_BAYS] =
{
    0,
    0,
    0
};

int stationBayCount = 1;

// =====================================================================
// Manual control
// =====================================================================

bool manualOverrideActive = false;
bool manualRelayState = false;

// =====================================================================
// Energy calculation
// =====================================================================

static unsigned long lastEnergyUpdateMs = 0;

void resetEnergy()
{
    energyWh = 0.0f;
    lastEnergyUpdateMs = millis();
}

void updateEnergy()
{
    unsigned long now = millis();

    if (lastEnergyUpdateMs == 0)
    {
        lastEnergyUpdateMs = now;
        return;
    }

    float deltaTimeHours =
        (now - lastEnergyUpdateMs) / 3600000.0f;

    lastEnergyUpdateMs = now;

    if (bayStatus == "CHARGING")
    {
        energyWh += power * deltaTimeHours;

        if (energyWh < 0.0f)
        {
            energyWh = 0.0f;
        }
    }
}
