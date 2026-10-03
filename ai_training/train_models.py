import pandas as pd

from sklearn.linear_model import (
    LogisticRegression,
    LinearRegression
)

from sklearn.model_selection import (
    train_test_split
)

from sklearn.metrics import (
    accuracy_score,
    classification_report,
    r2_score,
    mean_absolute_error
)

# ==============================================================
# EV Charging Optimizer - Model Training
# ==============================================================

DATASET = "synthetic_ev_data.csv"

FEATURES = [
    "hourOfDay",
    "dayOfWeek",
    "bayOccupied",
    "recentAvgCurrent",
    "sessionElapsedMin",
    "neighborBaysOccupied",
    "historicalArrivalRate",
]

# --------------------------------------------------------------
# Load dataset
# --------------------------------------------------------------

df = pd.read_csv(
    DATASET
)

print(
    f"Loaded {len(df)} records."
)

# ==============================================================
# ARRIVAL MODEL
# ==============================================================

X_arrival = df[
    FEATURES
]

y_arrival = df[
    "arrivedWithinWindow"
]

X_train, X_test, y_train, y_test = train_test_split(
    X_arrival,
    y_arrival,
    test_size=0.20,
    random_state=42,
    stratify=y_arrival
)

arrival_model = LogisticRegression(
    max_iter=1000
)

arrival_model.fit(
    X_train,
    y_train
)

arrival_prediction = (
    arrival_model.predict(
        X_test
    )
)

arrival_accuracy = (
    accuracy_score(
        y_test,
        arrival_prediction
    )
)

print()
print(
    "=============================="
)

print(
    "ARRIVAL MODEL"
)

print(
    "=============================="
)

print(
    f"Accuracy: {arrival_accuracy:.4f}"
)

print(
    "Intercept:",
    arrival_model.intercept_[0]
)

print(
    "Coefficients:"
)

for feature, coefficient in zip(
    FEATURES,
    arrival_model.coef_[0]
):
    print(
        f"  {feature}: {coefficient}"
    )

# ==============================================================
# DURATION MODEL
# ==============================================================

active = (
    df[
        df["bayOccupied"] == 1
    ]
)

X_duration = active[
    FEATURES
]

y_duration = active[
    "actualDurationMin"
]

X_train_d, X_test_d, y_train_d, y_test_d = train_test_split(
    X_duration,
    y_duration,
    test_size=0.20,
    random_state=42
)

duration_model = LinearRegression()

duration_model.fit(
    X_train_d,
    y_train_d
)

duration_prediction = (
    duration_model.predict(
        X_test_d
    )
)

duration_r2 = (
    r2_score(
        y_test_d,
        duration_prediction
    )
)

duration_mae = (
    mean_absolute_error(
        y_test_d,
        duration_prediction
    )
)

print()
print(
    "=============================="
)

print(
    "DURATION MODEL"
)

print(
    "=============================="
)

print(
    f"R2: {duration_r2:.4f}"
)

print(
    f"MAE: {duration_mae:.4f} minutes"
)

print(
    "Intercept:",
    duration_model.intercept_
)

print(
    "Coefficients:"
)

for feature, coefficient in zip(
    FEATURES,
    duration_model.coef_
):
    print(
        f"  {feature}: {coefficient}"
    )

# ==============================================================
# Save metrics
# ==============================================================

with open(
    "model_metrics.txt",
    "w"
) as f:

    f.write(
        "EV Charging Optimizer Model Metrics\n"
    )

    f.write(
        "===================================\n\n"
    )

    f.write(
        f"Dataset rows: {len(df)}\n"
    )

    f.write(
        f"Arrival accuracy: {arrival_accuracy:.4f}\n"
    )

    f.write(
        f"Duration R2: {duration_r2:.4f}\n"
    )

    f.write(
        f"Duration MAE: {duration_mae:.4f} minutes\n"
    )

print()
print(
    "Metrics saved to model_metrics.txt"
)
