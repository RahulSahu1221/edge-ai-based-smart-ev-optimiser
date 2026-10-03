import numpy as np
import pandas as pd

# ==============================================================
# EV Charging Optimizer - Synthetic Dataset Generator
# ==============================================================

SEED = 42
ROWS = 8000

rng = np.random.default_rng(SEED)

# --------------------------------------------------------------
# Time features
# --------------------------------------------------------------

hour = rng.integers(
    0,
    24,
    ROWS
)

day_of_week = rng.integers(
    0,
    7,
    ROWS
)

# --------------------------------------------------------------
# Bay occupancy
# --------------------------------------------------------------

bay_occupied = rng.integers(
    0,
    2,
    ROWS
)

# --------------------------------------------------------------
# Current
# --------------------------------------------------------------

recent_avg_current = (
    np.clip(
        rng.normal(
            8.0,
            5.0,
            ROWS
        ),
        0,
        32
    )
    * bay_occupied
)

# --------------------------------------------------------------
# Session elapsed time
# --------------------------------------------------------------

session_elapsed_min = (
    np.clip(
        rng.gamma(
            shape=2.0,
            scale=35.0,
            size=ROWS
        ),
        0,
        240
    )
    * bay_occupied
)

# --------------------------------------------------------------
# Neighbor occupancy
# --------------------------------------------------------------

neighbor_bays_occupied = rng.integers(
    0,
    4,
    ROWS
)

# --------------------------------------------------------------
# Historical arrival rate
# --------------------------------------------------------------

historical_arrival_rate = np.where(
    (hour >= 8) & (hour <= 10),
    0.62,
    np.where(
        (hour >= 18) & (hour <= 21),
        0.58,
        np.where(
            (hour >= 0) & (hour <= 5),
            0.06,
            np.where(
                (hour >= 11) & (hour <= 17),
                0.30,
                0.18
            )
        )
    )
)

# --------------------------------------------------------------
# Weekend adjustment
# --------------------------------------------------------------

weekend = (
    (day_of_week == 0)
    |
    (day_of_week == 6)
)

historical_arrival_rate = np.where(
    weekend,
    historical_arrival_rate * 0.85,
    historical_arrival_rate
)

# --------------------------------------------------------------
# Arrival target
# --------------------------------------------------------------

peak_hour = (
    ((hour >= 8) & (hour <= 10))
    |
    ((hour >= 18) & (hour <= 21))
)

logit = (
    -2.4
    + 1.8 * historical_arrival_rate
    + 0.035 * peak_hour
    - 0.12 * neighbor_bays_occupied
    - 0.35 * bay_occupied
    + 0.04 * (6 - np.abs(hour - 12))
    + 0.10 * (day_of_week < 5)
)

arrival_probability = (
    1.0
    /
    (
        1.0
        +
        np.exp(-logit)
    )
)

arrived_within_window = (
    rng.binomial(
        1,
        arrival_probability
    )
)

# --------------------------------------------------------------
# Session duration target
# --------------------------------------------------------------

actual_duration_min = np.clip(
    150
    - 0.45 * session_elapsed_min
    - 1.8 * recent_avg_current
    + 8 * weekend
    + rng.normal(
        0,
        18,
        ROWS
    ),
    5,
    180
)

# --------------------------------------------------------------
# Build dataframe
# --------------------------------------------------------------

df = pd.DataFrame(
    {
        "hourOfDay": hour,
        "dayOfWeek": day_of_week,
        "bayOccupied": bay_occupied,
        "recentAvgCurrent": recent_avg_current,
        "sessionElapsedMin": session_elapsed_min,
        "neighborBaysOccupied": neighbor_bays_occupied,
        "historicalArrivalRate": historical_arrival_rate,
        "arrivedWithinWindow": arrived_within_window,
        "actualDurationMin": actual_duration_min,
    }
)

# --------------------------------------------------------------
# Save
# --------------------------------------------------------------

df.to_csv(
    "synthetic_ev_data.csv",
    index=False
)

print(
    f"Generated {len(df)} synthetic EV charging records."
)

print(
    "Saved to synthetic_ev_data.csv"
)
