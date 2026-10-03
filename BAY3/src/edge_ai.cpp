#include <Arduino.h>
#include <time.h>

#include "edge_ai.h"
#include "State.h"
#include "Peripherals.h"
#include "model.h"

// =====================================================================
// Historical arrival rate
//
// Synthetic-data pattern specified by the SRS:
// 08:00-10:00 -> high demand
// 18:00-21:00 -> high demand
// 00:00-05:00 -> low demand
// =====================================================================

float historicalArrivalRate(
    int hour,
    int dayOfWeek)
{
    float rate;

    if (hour >= 8 && hour <= 10)
    {
        rate = 0.62f;
    }
    else if (hour >= 18 && hour <= 21)
    {
        rate = 0.58f;
    }
    else if (hour >= 0 && hour <= 5)
    {
        rate = 0.06f;
    }
    else if (hour >= 11 && hour <= 17)
    {
        rate = 0.30f;
    }
    else
    {
        rate = 0.18f;
    }

    // Slightly lower expected arrival rate on weekends.
    if (dayOfWeek == 0 || dayOfWeek == 6)
    {
        rate *= 0.85f;
    }

    return rate;
}

// =====================================================================
// Edge AI inference
// =====================================================================

void runEdgeAIInference()
{
    struct tm timeinfo;

    int hourOfDay = lastHourOfDay;
    int dayOfWeek = lastDayOfWeek;

    if (getLocalTime(&timeinfo, 100))
    {
        hourOfDay = timeinfo.tm_hour;
        dayOfWeek = timeinfo.tm_wday;
    }

    lastHourOfDay = hourOfDay;
    lastDayOfWeek = dayOfWeek;

    // -------------------------------------------------------------
    // Feature 1
    // -------------------------------------------------------------

    float hourFeature =
        (float)hourOfDay;

    // -------------------------------------------------------------
    // Feature 2
    // -------------------------------------------------------------

    float dayFeature =
        (float)dayOfWeek;

    // -------------------------------------------------------------
    // Feature 3
    // current bay status
    // -------------------------------------------------------------

    float bayOccupied =
        (bayStatus == "CHARGING")
        ? 1.0f
        : 0.0f;

    // -------------------------------------------------------------
    // Feature 4
    // rolling average current
    // -------------------------------------------------------------

    float avgCurrent =
        recentAvgCurrent();

    // -------------------------------------------------------------
    // Feature 5
    // session elapsed time
    // -------------------------------------------------------------

    float sessionElapsedMin = 0.0f;

    if (bayStatus == "CHARGING" &&
        sessionStartMs > 0)
    {
        sessionElapsedMin =
            (millis() - sessionStartMs)
            / 60000.0f;
    }

    // -------------------------------------------------------------
    // Feature 6
    // neighbor bay occupancy
    // -------------------------------------------------------------

    float neighborsOccupied =
        (float)neighborBaysOccupied;

    // -------------------------------------------------------------
    // Feature 7
    // historical arrival rate
    // -------------------------------------------------------------

    float histRate =
        historicalArrivalRate(
            hourOfDay,
            dayOfWeek
        );

    // -------------------------------------------------------------
    // AI inference
    // -------------------------------------------------------------

    unsigned long startMicros = micros();

    predictedArrivalProb =
        predictArrival(
            hourFeature,
            dayFeature,
            bayOccupied,
            avgCurrent,
            sessionElapsedMin,
            neighborsOccupied,
            histRate
        );

    if (bayStatus == "CHARGING")
    {
        float totalPredictedMin =
            predictDuration(
                hourFeature,
                dayFeature,
                bayOccupied,
                avgCurrent,
                sessionElapsedMin,
                neighborsOccupied,
                histRate
            );

        // -----------------------------------------------------------
        // SRS Table 9 defines predictedDurationMin as the estimated
        // REMAINING charging time, not the total session length, so
        // the already-elapsed session time is subtracted out here.
        // -----------------------------------------------------------

        float remainingMin =
            totalPredictedMin - sessionElapsedMin;

        predictedDurationMin =
            (int) round(remainingMin);

        if (predictedDurationMin < 0)
        {
            predictedDurationMin = 0;
        }
    }
    else
    {
        predictedDurationMin = 0;
    }

    unsigned long elapsedMicros =
        micros() - startMicros;

    Serial.print(
        "[AI] Arrival probability = "
    );

    Serial.println(
        predictedArrivalProb,
        3
    );

    Serial.print(
        "[AI] Predicted remaining duration = "
    );

    Serial.print(
        predictedDurationMin
    );

    Serial.println(" min");

    Serial.print(
        "[AI] Inference time = "
    );

    Serial.print(
        elapsedMicros / 1000.0f,
        3
    );

    Serial.println(" ms");
}
