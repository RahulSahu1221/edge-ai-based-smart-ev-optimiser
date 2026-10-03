#ifndef STATE_H
#define STATE_H

#include <Arduino.h>
#include "config.h"

// =====================================================================
// Energy
// =====================================================================

void updateEnergy();
void resetEnergy();

// =====================================================================
// Live bay state
// =====================================================================

extern String bayStatus;

extern float voltage;
extern float current;
extern float power;
extern float energyWh;
extern float temperature;

extern unsigned long sessionStartMs;

// =====================================================================
// Edge AI
// =====================================================================

extern float predictedArrivalProb;
extern int predictedDurationMin;

extern int lastHourOfDay;
extern int lastDayOfWeek;

// =====================================================================
// Optimization
// =====================================================================

extern String loadDecision;
extern String optimizationReason;

extern int throttleLevel;

// =====================================================================
// Shared configuration
// =====================================================================

extern float predictionThreshold;

extern int peakTariffStartHr;
extern int peakTariffEndHr;

extern float overloadCurrentA;
extern float maxStationLoadW;

extern float overvoltageThresholdV;
extern int maxSessionDurationMin;

// =====================================================================
// Safety / fault state
// =====================================================================

extern bool overloadActive;
extern bool overvoltageActive;
extern bool sensorFault;
extern bool stuckSession;

// =====================================================================
// Multi-bay station context
// =====================================================================

extern float stationTotalPowerW;

extern int neighborBaysOccupied;

extern float neighborPower[MAX_NEIGHBOR_BAYS];

extern int neighborDurationMin[MAX_NEIGHBOR_BAYS];

extern int stationBayCount;

// =====================================================================
// Manual control
// =====================================================================

extern bool manualOverrideActive;
extern bool manualRelayState;

#endif
