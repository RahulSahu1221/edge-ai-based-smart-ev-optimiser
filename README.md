<div align="center">

# Edge AI Based Smart EV Charging Optimizer

*Intelligent, On-Device Charging Control for a Multi-Bay EV Station.*

A simulated three-bay electric vehicle charging station in which every bay runs **on-device Edge AI**
to predict arrival demand and session duration, and a **local optimization algorithm** to allow,
throttle, or defer charging in real time — all coordinated through **ThingsBoard** over MQTT.

Built entirely on an **ESP32**, simulated in **Wokwi**, with no machine-learning runtime required
on the device itself.

![C++](https://img.shields.io/badge/C%2B%2B-ESP32_Firmware-00599C?style=for-the-badge)
![PlatformIO](https://img.shields.io/badge/Build-PlatformIO-FF7F00?style=for-the-badge)
![MQTT](https://img.shields.io/badge/Protocol-MQTT-660066?style=for-the-badge)
![ThingsBoard](https://img.shields.io/badge/Cloud-ThingsBoard-2C8F7C?style=for-the-badge)
![Edge AI](https://img.shields.io/badge/AI-Edge_Inference-E74C3C?style=for-the-badge)
![Python](https://img.shields.io/badge/Training-Python-3776AB?style=for-the-badge)
![Wokwi](https://img.shields.io/badge/Simulation-Wokwi-00ACC1?style=for-the-badge)

---

### Repository Contents

| Path | Contents |
|---|---|
| `BAY1/` | Complete PlatformIO firmware project — build and simulate directly from here |
| `ai_training/` | Synthetic dataset generator and model training scripts for the Edge AI |
| `EV_Charging_Optimizer_Project_Documentation.docx` | Full technical project report |

**BAY2 and BAY3 run the identical firmware in `BAY1/`.** The only difference between any two bays
is each device's own `include/secrets.h` — its ThingsBoard access token and bay ID. Nothing else
in the source changes between bays.

</div>

---

## Overview

Multi-bay EV charging stations typically allocate power with simple, static rules and have no way
to anticipate demand. This project explores a lighter-weight alternative: run a small, trained
model directly on the same low-cost microcontroller that already measures voltage and current, and
let it inform — not replace — a deterministic, explainable decision layer on top.

Each bay follows three stages on every cycle:

1. **Input** — sample voltage, current, power, energy, and temperature; track plug-in/plug-out state.
2. **Intelligence** — an on-device Edge AI model predicts the probability of a new arrival and the
   expected remaining duration of the current session.
3. **Action** — a priority-ordered optimization algorithm converts those predictions, together with
   live readings and station-wide context, into an `ALLOW`, `THROTTLE`, or `DEFER` decision.

## System Architecture

```
  Sensors (V / I / Temp / Plug)
            |
            v
     ESP32 Edge Layer  ---- Edge AI inference (on-device)
            |                 |
            v                 v
   Local Optimization  <-- predictions + station context
            |
            v
     Relay / LED Output
            |
            v
   MQTT  --------------->  ThingsBoard Cloud
(telemetry, RPC, shared     (dashboards, alarms,
     attributes)             remote control)
```

Three bays (`BAY1`, `BAY2`, `BAY3`) run independently but share station-wide context through
ThingsBoard shared attributes, so a station-wide power limit and session-priority comparisons work
across all three.

## Hardware

| Component | Role |
|---|---|
| ESP32 DevKit (Wokwi simulated) | Per-bay controller — sensing, Edge AI, MQTT |
| Potentiometer x2 | Simulated voltage (0–250 V) and current (0–32 A) input |
| DHT22 | Temperature sensing and sensor-fault detection |
| Relay module | Charging current on/off, duty-cycled for partial throttle |
| LEDs (green / yellow / red) | Local FREE / CHARGING / FAULT indication |
| Push buttons x2 | Simulated EV plug-in and plug-out events |

Full pin mapping is in `BAY1/include/config.h` and the wiring diagram in `BAY1/test/diagram.json`.

## Firmware Modules (`BAY1/`)

| Module | Responsibility |
|---|---|
| `State` | All live bay variables, configuration defaults, energy accumulation |
| `Peripherals` | Sensor sampling, plug-in/plug-out handling |
| `Network` | Wi-Fi and MQTT connection management |
| `Telemetry` | Builds and publishes the telemetry payload |
| `Attributes` | Requests/applies shared configuration from ThingsBoard |
| `Edge AI` | Feature construction and on-device inference |
| `Optimization` | The ALLOW / THROTTLE / DEFER decision algorithm and relay control |
| `RPC` | Remote commands: relay control, manual throttle, status, override clear |

## Edge AI

Two lightweight models, both converted to plain C/C++ arithmetic so no ML runtime runs on the
ESP32:

- **Arrival model** (Logistic Regression) — predicts the probability an EV arrives soon.
- **Duration model** (Linear Regression) — predicts the remaining minutes of an active session.

Both are trained on the same 7 features (time of day, day of week, bay occupancy, recent current,
session elapsed time, neighbor bay occupancy, and a historical arrival-rate pattern) using a
synthetic 8,000-record dataset. See `ai_training/` for the generator and training script, and
`ai_training/model_metrics.txt` for measured performance:

| Model | Metric | Value |
|---|---|---|
| Arrival (Logistic Regression) | Test accuracy | 0.8888 |
| Duration (Linear Regression) | Test R² | 0.6421 |
| Duration (Linear Regression) | Test MAE | 13.99 minutes |

## Getting Started

### Firmware (BAY1 / BAY2 / BAY3)

```bash
# 1. Open BAY1/ directly in VS Code with the PlatformIO extension installed.

# 2. Create your device secrets (not committed to git):
cd BAY1/include
cp secrets.h
# edit secrets.h with this bay's real Wi-Fi credentials, ThingsBoard token, and BAY_ID

# 3. Build and simulate
# Open BAY1/test/diagram.json and click "Start Simulation" (Wokwi extension),
# or build/upload normally via PlatformIO for real ESP32 hardware.
```

For BAY2 and BAY3, copy the entire `BAY1/` project and repeat only step 2 with that bay's own
token and ID — no other file changes are needed.

### Edge AI training pipeline

```bash
cd ai_training
pip install -r requirements.txt
python generate_dataset.py
python train_models.py
```

This regenerates `synthetic_ev_data.csv` and prints the trained coefficients and metrics. The
coefficients currently deployed in `BAY1/src/model.h` were produced by this exact pipeline.

## Dashboard

A live ThingsBoard dashboard displays, per bay: voltage, current, power, temperature, energy,
bay status, Edge AI predictions, load decision, and throttle level, alongside manual relay and
throttle controls bound to the device's RPC methods, and a station configuration panel for the
shared attributes (station load limit, overload current, peak tariff window, and related
thresholds).

## Limitations

- Validated in Wokwi circuit simulation; not yet deployed on physical charging hardware.
- Voltage and current are simulated via potentiometers rather than calibrated sensors.
- Both Edge AI models are trained on a synthetic dataset rather than real session data.
- MQTT currently uses plaintext transport, suitable for the simulated prototype only.

## Future Scope

- Deploy on physical ESP32 hardware with real current/voltage sensing.
- Retrain periodically on real collected session data.
- Enable TLS-secured MQTT.
- Extend optimization to recommend a specific bay to an arriving EV, not only react after
  occupancy.

## Documentation

The complete technical report — requirements, architecture, mathematical model, testing, and
results — is in `EV_Charging_Optimizer_Project_Documentation.docx`.

## Member

Rahul Sahu
Internship Project — Emertxe IoT Internship 2026
