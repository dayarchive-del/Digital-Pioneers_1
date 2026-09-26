# 🪔 Smart Solar Agarbatti Drying Chamber — Digital-Pioneers_1

> **SIH 2026 Prototype | ESP32 Smart Drying + YOLOv8 + OpenCV + Rule-Based Grading**
>
> “Our system uses ESP32-based smart drying, manual placement under a camera for inspection, YOLOv8 for visual defect detection, OpenCV for measurable quality features, and a rule-based decision engine to produce an explainable final grade.”

[![SIH 2026](https://img.shields.io/badge/SIH-2026-orange)]()
[![ESP32](https://img.shields.io/badge/ESP32-Arduino-blue)]()
[![YOLOv8](https://img.shields.io/badge/YOLOv8-Ultralytics-green)]()
[![OpenCV](https://img.shields.io/badge/OpenCV-Measurement-red)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow)]()

---

## 📌 Problem

Traditional agarbatti (incense stick) sun-drying is:
- Weather-dependent & unhygienic (dust, insects, rain)
- Uneven drying → breakage, bending, fungus, poor burning
- No objective quality grading — fully manual visual check
- High wastage for small / rural manufacturers

## 💡 Solution

A **solar-powered smart drying chamber + AI quality inspection**:

```text
              SOLAR PANEL
                   ↓
                BATTERY
                   ↓
                 ESP32
              ↙          ↘
    TEMP/HUMIDITY       FAN / HEATER / LED / BUZZER
              ↓              ↓
          DRYING CHAMBER
                   ↓
                CAMERA
         (manual placement, no conveyor)
                   ↓
            ORIGINAL IMAGE
                   ↓
              YOLOv8
       Ready / Broken / Bent
                   ↓
          YOLO bounding boxes
                   ↓
               OpenCV
    width / length / colour / brightness
                   ↓
           FEATURE RECORD
                   +
           ESP32 DRYING STATUS
                   ↓
        RULE-BASED DECISION ENGINE
                   ↓
    ┌─────────────┼─────────────┐
    ↓             ↓             ↓
 GRADE A       GRADE C        REJECT
 ACCEPT      KEEP/CHECK      REMOVE
```

**Key design honesty (for judges):**
- YOLO = visual recognition only (`Ready`, `Broken`, `Bent`)
- OpenCV = deterministic measurements (pixels, aspect-ratio, brightness)
- ESP32 temp/RH = drying status (`READY_TO_INSPECT` / `UNDER_DRYING` / `UNKNOWN`)
- Decision engine = plain Python rules, **not a second AI model**
- Camera does **NOT** directly measure internal moisture
- Report mm only after calibration, else report pixels

---

## 📁 Repo Structure

```text
Digital-Pioneers_1/
├── README.md
├── TODO.md                        ← what is still pending
├── requirements.txt
├── LICENSE
├── .gitignore
│
├── ai-pipeline/
│   ├── Smart_Agarbatti_FINAL_Project_Pipeline_no_conveyor.ipynb  ← MAIN notebook
│   └── Smart_Agarbatti_NoComments_Pipeline.ipynb                 ← clean demo version
│
├── dashboard/
│   ├── smart-agarbatti-dashboard.html  ← double-click to open, needs internet (CDN)
│   └── smart-agarbatti-dashboard.jsx   ← React source
│
├── simulation/
│   └── Smart-Solar-Agarbatti-Drying-Chamber-Working-Model.html ← 3D + ESP32 logic sim
│
├── firmware/
│   └── smart_drying_chamber.ino   ← ESP32 Arduino sketch (mirrors simulation)
│
├── docs/
│   └── SIH2026-IDEA-Presentation-Format.pdf
│
└── assets/
    └── screenshot.png
```

---

# PART A — HARDWARE

This is the physical prototype: solar power + ESP32 sensing + controlled drying chamber.

## A.1 Block Diagram

```text
SOLAR PANEL (10-20 W, charging demo)
   ↓
BATTERY (sized for demo runtime)
   ↓
ESP32
 ├── INPUT: DHT22 Temp/RH (GPIO 4), START(32) / STOP(33) / RESET(23) buttons
 ├── DISPLAY: OLED SSD1306 (SDA 21 / SCL 22)
 └── OUTPUT: Heater relay (25), Circulation fan (26), Exhaust fan (27),
             LEDs R(16)/Y(17)/G(18), Buzzer (19)
   ↓
DRYING CHAMBER (insulated box + heater + fans + sliding trolleys)
   ↓
MANUAL PLACEMENT UNDER CAMERA (no conveyor)
```

## A.2 Components (BOM)

| Part | Notes |
|---|---|
| ESP32 DevKit | main controller, see `firmware/smart_drying_chamber.ino` |
| DHT22 | temp + humidity, GPIO 4 |
| OLED 128x64 SSD1306 | SDA 21 / SCL 22, shows T/RH + state + `DRYING_STATUS` |
| Heater + relay module | GPIO 25, opto-isolated, fused |
| Circulation fan | GPIO 26, always ON during heating stages |
| Exhaust fan | GPIO 27, ON when RH high / stages 2-3 / cooling |
| LEDs R/Y/G | GPIO 16/17/18, red=ERROR, yellow=drying, green=READY |
| Buzzer | GPIO 19, beeps on READY |
| Push buttons | START 32, STOP 33, RESET 23 (INPUT_PULLUP) |
| Solar panel 10-20 W + charge controller + battery | charging demo only — cannot continuously run heater |
| Chamber box + trays/trolleys | sliding trolleys, auto-eject message on READY |
| Camera (USB / phone) | fixed stand, plain contrasting background |

## A.3 Control Logic (state machine)

`IDLE → STAGE1 (≥45°C) → STAGE2 (≥50°C) → STAGE3 (≥52°C AND RH≤35%) → COOLING (≤30°C) → READY`, plus `ERROR` if `>60°C`. Hysteresis `±1°C`.

| State | Heater | Circ | Exhaust | LED |
|---|---|---|---|---|
| IDLE | OFF | OFF | OFF | OFF |
| STAGE1 | ON | ON | ON if RH>60% | Yellow |
| STAGE2 | ON | ON | ON | Yellow |
| STAGE3 | ON | ON | ON | Yellow |
| COOLING | OFF | ON | ON | Yellow |
| READY | OFF | OFF | OFF | Green + beep |
| ERROR | OFF | ON | ON | Red + buzzer |

Firmware: `firmware/smart_drying_chamber.ino`. Upload via Arduino IDE (ESP32 core + `DHT sensor` + `Adafruit SSD1306` + `Adafruit GFX` libs). Serial @115200 prints `T, RH, state, dry=` every 2 s where `dry` is `READY_TO_INSPECT` (READY) / `UNDER_DRYING` (stages) / `UNKNOWN` (ERROR) — this string is the input to the Software pipeline.

## A.4 Hardware Safety / Limits

- Relay isolation + fuse + ventilation mandatory. Test over-temp cutoff (>60°C → ERROR) before demo.
- Solar sizing not validated for continuous heater load — report battery voltage + runtime honestly.
- Simulation ≠ thermodynamics (see Part C).

---

# PART B — SOFTWARE

This is the AI inspection: camera image → YOLOv8 → OpenCV → rule-based grade.

## B.1 Pipeline Overview

```text
CAMERA IMAGE (manual placement)
   ↓
YOLOv8 — visual recognition only
 Detects: Ready / Broken / Bent (+ box + confidence)
   ↓
OpenCV on each YOLO ROI — deterministic measurement only
 Measures: width_px / height_px / aspect_ratio / colour (BGR/HSV) / brightness
   ↓
ESP32 DRYING_STATUS (from Part A serial/Wi-Fi)
 READY_TO_INSPECT / UNDER_DRYING / UNKNOWN
   ↓
RULE-BASED DECISION ENGINE (plain Python, NOT a second AI model)
   ↓
GRADE A (ACCEPT) / GRADE C (KEEP DRYING / DOWNGRADE) / REJECT (REMOVE) / MANUAL CHECK
```

Design honesty for judges:
- YOLO ≠ moisture meter. Under-dried is NOT a camera class — it comes from ESP32 temp/RH.
- OpenCV keeps original resolution (no 640x640 resize before measuring).
- Report mm only after calibration, else pixels.

## B.2 AI Notebook

File: `ai-pipeline/Smart_Agarbatti_FINAL_Project_Pipeline_no_conveyor.ipynb` (main) + `Smart_Agarbatti_NoComments_Pipeline.ipynb` (clean demo).

Setup:
```bash
pip install -r requirements.txt
# ultralytics opencv-python matplotlib pandas pyyaml numpy
```

Dataset (exact IDs required):
```text
Agarbatti_Dataset/
├── train/images/ + train/labels/
├── val/images/ + val/labels/
└── data.yaml   # 0=Ready, 1=Broken, 2=Bent
```

Train:
```python
from ultralytics import YOLO
model = YOLO("yolov8n.pt")
model.train(data="data.yaml", epochs=50, imgsz=640, batch=8,
            project="agarbatti_runs", name="quality_grading", patience=15)
# → agarbatti_runs/quality_grading/weights/best.pt
```

Run:
```python
MODEL_PATH = "agarbatti_runs/quality_grading/weights/best.pt"
IMAGE_PATH = "agarbatti_image.jpg"
DRYING_STATUS = "READY_TO_INSPECT"  # or UNDER_DRYING / UNKNOWN
results, final_records = complete_agarbatti_pipeline(IMAGE_PATH, model, DRYING_STATUS)
```

Per-stick output: `class, confidence, box, width_px, height_px, aspect_ratio, width_mm/height_mm (if PIXELS_PER_MM set), mean BGR/HSV, brightness, status, grade, action, reason`.

Grading table:

| Condition | Grade | Action |
|---|---|---|
| Broken | REJECT | REMOVE |
| Bent | REJECT | REMOVE |
| `UNDER_DRYING` | GRADE C | KEEP DRYING |
| `UNKNOWN` | MANUAL CHECK | CHECK SENSORS |
| confidence < 50% | MANUAL CHECK | RECHECK IMAGE |
| Ready but width/length/brightness outside calibrated range | GRADE C | DOWNGRADE / CHECK |
| Ready + drying complete + all checks pass | GRADE A | ACCEPT |

Keep `MIN_WIDTH_PX etc = None` until measured from 30+ real good samples. For mm: `PIXELS_PER_MM` via ruler/ArUco at inspection plane (e.g. 10 mm = 120 px → 12.0).

## B.3 Dashboard

Files: `dashboard/smart-agarbatti-dashboard.html` (double-click, needs internet for CDN) + `.jsx` source.

- `qualityDecision()` = JS port of notebook `quality_decision()` — same grades.
- Real in-browser measurement (greyscale + threshold + bbox + brightness) = stand-in for `measure_detected_object` + `extract_colour_features`.
- ONLY mock part: `mockYoloClassify()` until `best.pt` is trained. Replace with:
```js
const form = new FormData();
form.append("image", imageBlob);
const res = await fetch("/api/inspect", { method: "POST", body: form });
const data = await res.json();
return { class: data.prediction, confidence: data.confidence };
```
- Features: upload/camera, threshold sliders, batch pie/bar/line charts, history, export.

---

# PART C — SIMULATION (IMPORTANT)

> This is the primary visual demo when physical hardware cannot run continuously. Judges must understand what it proves and what it does NOT prove.

File: `simulation/working-model.html` (Three.js r128 via CDN, open directly in browser).

## C.1 What It Is

Faithful **ESP32 control-logic + chamber visualisation**, NOT a physics engine:
- 3D chamber: walls, heater, fans, trolleys, sensor point, airflow indicators
- Same state machine + thresholds + pin map as `firmware/smart_drying_chamber.ino`
- Proves: stage transitions, fan/heater/LED/buzzer logic, trolley auto-eject on READY, over-temp ERROR, serial-monitor logging

## C.2 How To Demo (2 min)

1. Open file → views: Normal / Exploded / Cutaway, toggle Labels, drag-rotate / scroll-zoom, click part for label.
2. Press START → watch `IDLE → STAGE1 → STAGE2 → STAGE3 → COOLING → READY`, temp/RH/chart live, chips for heater/fans/LED.
3. Drag Sunlight slider → battery % changes. Switch Time-scale 1x/5x/20x to fast-forward.
4. Toggle Manual sensor override → drag Temp/RH sliders to force STAGE jumps / force ERROR (>60°C).
5. Press “Slide trolleys out”, then “Run test sequence” → green checks in test runner.
6. Point to Serial Monitor log + footer disclaimer: *“Simulation of control logic only. It does not model real heat transfer or moisture loss.”* — keep this line, it builds trust.

## C.3 What It Is NOT

- No heat-transfer / CFD / moisture-diffusion modelling.
- No solar-yield or battery-discharge modelling beyond indicator.
- Drying times are accelerated for demo, not real kinetics.
- Always pair with Part A (real ESP32 serial log) + Part B (real inspection table) to show sim → real mapping.

## C.4 Sim ↔ Real Mapping

| Simulation element | Real counterpart |
|---|---|
| `updateController()` JS | `loop()` in `firmware/smart_drying_chamber.ino` |
| Temp/RH sliders (manual) | DHT22 readings |
| Heater/fan chips | Relay GPIOs 25/26/27 |
| Stage label + chart | OLED + Serial prints |
| Trolley auto-eject message | Operator unloads on READY beep |
| `ERROR` on >60°C | Same cutoff in firmware |

---

## 🚀 Quick Demo for Judges (5 min)

1. Open `simulation/...Working-Model.html` → START → show STAGE1→2→3→COOLING→READY, trolley auto-eject.
2. Open `dashboard/smart-agarbatti-dashboard.html` → upload 3 photos (Ready/Broken/Bent) → show grades + reasons + batch summary.
3. Open `ai-pipeline/...FINAL...ipynb` in Colab → Run All → show DataFrame + YOLO boxes + batch summary.
4. Show ESP32 + chamber (or photo/video) + `firmware/*.ino` serial log.
5. One-liner: *“ESP32 smart drying, manual camera placement, YOLOv8 defect detection, OpenCV measurements, rule-based explainable grading.”*
6. Show `TODO.md` calibration checklist — judges love honesty about limits.

---

## ⚠️ Limitations (state explicitly)

- No trained `best.pt` yet — dashboard uses mock classifier, notebook needs real dataset.
- No pixel→mm calibration yet — report pixels until ruler/ArUco calibration done.
- Solar sizing not validated for continuous heater load.
- Simulation ≠ thermodynamics.
- Camera ≠ moisture meter — drying status comes from ESP32, not vision.

---

## 👥 Team — SIH 2026

- Presentation template: `docs/SIH2026-IDEA-Presentation-Format.pdf`
- Pending work: see `TODO.md`

## 📄 License

MIT — see `LICENSE`.
