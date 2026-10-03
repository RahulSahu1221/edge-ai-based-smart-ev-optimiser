#include <Arduino.h>

#include "optimization.h"
#include "config.h"
#include "State.h"

// =====================================================================
// Calculate median of valid neighbor session durations
// =====================================================================

int calculateMedianNeighborDuration()
{
    int values[MAX_NEIGHBOR_BAYS];

    int count = 0;

    for (int i = 0;
         i < MAX_NEIGHBOR_BAYS;
         i++)
    {
        if (neighborDurationMin[i] > 0)
        {
            values[count] =
                neighborDurationMin[i];

            count++;
        }
    }

    // No neighbor duration available.
    if (count == 0)
    {
        return 0;
    }

    // Simple insertion sort.
    for (int i = 1; i < count; i++)
    {
        int key = values[i];

        int j = i - 1;

        while (j >= 0 &&
               values[j] > key)
        {
            values[j + 1] = values[j];
            j--;
        }

        values[j + 1] = key;
    }

    // Odd number of values.
    if (count % 2 == 1)
    {
        return values[count / 2];
    }

    // Even number of values.
    return
        (values[count / 2 - 1] +
         values[count / 2]) / 2;
}

// =====================================================================
// Main local optimization algorithm
//
// Follows SRS Section 8.7:
//
// 1. Read local status/power/predictions.
// 2. Read neighbor context.
// 3. Calculate total station power.
// 4. Overcurrent -> THROTTLE 50%.
// 5. Station overload -> compare predicted duration with median.
// 6. Peak tariff + FREE + low predicted arrival -> DEFER.
// 7. Otherwise -> ALLOW.
// =====================================================================

void runOptimization()
{
    // -------------------------------------------------------------
    // Safety: sensor fault
    // -------------------------------------------------------------

    if (sensorFault)
    {
        bayStatus = "FAULT";

        loadDecision = "DEFER";
        optimizationReason = "SENSOR_FAULT";

        throttleLevel = 0;

        digitalWrite(
            RELAY_PIN,
            LOW
        );

        return;
    }

    // -------------------------------------------------------------
    // Safety: overvoltage
    // -------------------------------------------------------------

    if (voltage > overvoltageThresholdV)
    {
        overvoltageActive = true;

        bayStatus = "FAULT";

        loadDecision = "DEFER";
        optimizationReason = "OVERVOLTAGE";

        throttleLevel = 0;

        digitalWrite(
            RELAY_PIN,
            LOW
        );

        return;
    }

    overvoltageActive = false;

    // -------------------------------------------------------------
    // Calculate station-wide power
    //
    // local bay + all known neighbor bay powers
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
    // Safety: local overcurrent
    // -------------------------------------------------------------

    if (bayStatus == "CHARGING" &&
        current > overloadCurrentA)
    {
        overloadActive = true;

        loadDecision = "THROTTLE";
        optimizationReason = "OVERCURRENT";

        throttleLevel = 50;

        Serial.println(
            "[OPT] OVERCURRENT -> THROTTLE 50%"
        );

        return;
    }

    overloadActive = false;

    // -------------------------------------------------------------
    // Stuck session check
    // -------------------------------------------------------------

    if (bayStatus == "CHARGING" &&
        sessionStartMs > 0)
    {
        float elapsedMin =
            (millis() - sessionStartMs)
            / 60000.0f;

        if (elapsedMin >
            maxSessionDurationMin)
        {
            stuckSession = true;

            loadDecision = "THROTTLE";
            optimizationReason = "STUCK_SESSION";

            throttleLevel = 50;

            Serial.println(
                "[OPT] STUCK SESSION -> THROTTLE"
            );

            return;
        }
        else
        {
            stuckSession = false;
        }
    }
    else
    {
        stuckSession = false;
    }

    // -------------------------------------------------------------
    // FREE bay
    //
    // Peak tariff + low arrival probability -> DEFER
    // -------------------------------------------------------------

    if (bayStatus == "FREE")
    {
        bool peakHour =
            (
                lastHourOfDay >=
                peakTariffStartHr
            )
            &&
            (
                lastHourOfDay <=
                peakTariffEndHr
            );

        if (peakHour &&
            predictedArrivalProb <
                predictionThreshold)
        {
            loadDecision = "DEFER";

            optimizationReason =
                "PEAK_TARIFF_LOW_DEMAND";

            throttleLevel = 0;
        }
        else
        {
            loadDecision = "ALLOW";

            optimizationReason =
                "NORMAL";

            throttleLevel = 0;
        }

        return;
    }

    // -------------------------------------------------------------
    // Only charging bays continue with charging optimization
    // -------------------------------------------------------------

    if (bayStatus != "CHARGING")
    {
        loadDecision = "DEFER";

        optimizationReason =
            "BAY_NOT_CHARGING";

        throttleLevel = 0;

        return;
    }

    // -------------------------------------------------------------
    // Station-wide peak-load management
    // -------------------------------------------------------------

    if (stationTotalPowerW >
        maxStationLoadW)
    {
        int medianDuration =
            calculateMedianNeighborDuration();

        // If there are no valid neighbor durations,
        // avoid making an arbitrary priority decision.
        if (medianDuration > 0 &&
            predictedDurationMin >
                medianDuration)
        {
            loadDecision = "THROTTLE";

            optimizationReason =
                "STATION_LOAD_LIMIT";

            throttleLevel = 70;

            Serial.print(
                "[OPT] Station power = "
            );

            Serial.print(
                stationTotalPowerW
            );

            Serial.println(
                " W -> THROTTLE 70%"
            );
        }
        else
        {
            loadDecision = "ALLOW";

            optimizationReason =
                "STATION_LOAD_PRIORITY";

            throttleLevel = 100;
        }

        return;
    }

    // -------------------------------------------------------------
    // Normal operation
    // -------------------------------------------------------------

    loadDecision = "ALLOW";

    optimizationReason = "NORMAL";

    throttleLevel = 100;
}

// =====================================================================
// Relay duty-cycle controller
//
// 100% -> ON
// 0%   -> OFF
// Intermediate -> time-proportional duty cycle
// =====================================================================

void applyRelayDutyCycle()
{
    // -------------------------------------------------------------
    // Safety always has priority
    // -------------------------------------------------------------

    if (sensorFault ||
        overvoltageActive)
    {
        digitalWrite(
            RELAY_PIN,
            LOW
        );

        return;
    }

    // -------------------------------------------------------------
    // Manual override
    // -------------------------------------------------------------

    if (manualOverrideActive)
    {
        if (manualRelayState)
        {
            digitalWrite(
                RELAY_PIN,
                HIGH
            );
        }
        else
        {
            digitalWrite(
                RELAY_PIN,
                LOW
            );
        }

        return;
    }

    // -------------------------------------------------------------
    // Non-charging state
    // -------------------------------------------------------------

    if (bayStatus != "CHARGING")
    {
        digitalWrite(
            RELAY_PIN,
            LOW
        );

        return;
    }

    // -------------------------------------------------------------
    // Full power
    // -------------------------------------------------------------

    if (throttleLevel >= 100)
    {
        digitalWrite(
            RELAY_PIN,
            HIGH
        );

        return;
    }

    // -------------------------------------------------------------
    // Fully disabled
    // -------------------------------------------------------------

    if (throttleLevel <= 0)
    {
        digitalWrite(
            RELAY_PIN,
            LOW
        );

        return;
    }

    // -------------------------------------------------------------
    // Duty-cycle throttling
    // -------------------------------------------------------------

    const unsigned long PERIOD_MS =
        2000UL;

    unsigned long phase =
        millis() % PERIOD_MS;

    unsigned long onTime =
        (PERIOD_MS *
         (unsigned long)throttleLevel)
        / 100UL;

    digitalWrite(
        RELAY_PIN,
        phase < onTime
        ? HIGH
        : LOW
    );
}

// =====================================================================
// LED status
//
// Green  = FREE
// Yellow = CHARGING
// Red    = FAULT / OVERLOAD
// =====================================================================

void updateLeds()
{
    // -------------------------------------------------------------
    // Fault
    // -------------------------------------------------------------

    if (bayStatus == "FAULT" ||
        overloadActive ||
        overvoltageActive ||
        sensorFault)
    {
        digitalWrite(
            LED_GREEN,
            LOW
        );

        digitalWrite(
            LED_YELLOW,
            LOW
        );

        digitalWrite(
            LED_RED,
            HIGH
        );

        return;
    }

    // -------------------------------------------------------------
    // Free
    // -------------------------------------------------------------

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

        return;
    }

    // -------------------------------------------------------------
    // Charging
    // -------------------------------------------------------------

    if (bayStatus == "CHARGING")
    {
        digitalWrite(
            LED_GREEN,
            LOW
        );

        digitalWrite(
            LED_YELLOW,
            HIGH
        );

        digitalWrite(
            LED_RED,
            LOW
        );

        return;
    }

    // Unknown state -> safe indication
    digitalWrite(
        LED_GREEN,
        LOW
    );

    digitalWrite(
        LED_YELLOW,
        LOW
    );

    digitalWrite(
        LED_RED,
        HIGH
    );
}
