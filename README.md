# Smart Solar Agarbatti Drying Chamber — Digital Pioneers
**Smart India Hackathon 2026 | ESP32 Smart Drying + YOLOv8 + OpenCV Quality Grading**

> Solar-powered drying chamber for agarbatti manufacturers with staged ESP32 process control and vision-based grading (Ready / Broken / Bent → Grade A / Grade C / Reject).

## Live Demo

- 3D Working Model Simulation: https://dayarchive-del.github.io/Digital-Pioneers_1/simulation/working-model.html
- Inspection Dashboard: https://dayarchive-del.github.io/Digital-Pioneers_1/dashboard/smart-agarbatti-dashboard.html

No install needed — both run directly in the browser (internet required for CDN libraries). Press START in the simulation to run the full drying cycle.

## Problem

Traditional agarbatti sun drying is weather dependent and unhygienic. Uneven drying causes breakage, bending and poor burning, and grading is done manually without any objective record. Small and rural units need a low-cost alternative that works off-grid.

## Solution

A single workflow covering drying and grading:

```
Solar Panel → Battery → ESP32 → Drying Chamber → Camera
                              (DHT22 temp/RH feedback)
Camera Image → YOLOv8 (Ready/Broken/Bent) → OpenCV measurements
  + ESP32 Drying Status → Decision Engine → Grade A / Grade C / Reject
```

Role split: YOLO handles visual recognition only. OpenCV handles deterministic measurement (size, colour, brightness) on the detected region. The ESP32 temperature/humidity logic supplies drying status. A rule-based engine combines the three into a final grade with reason and action. The camera does not measure internal moisture; under-drying is reported by the sensors, not by vision.

## Repo Structure

```
Digital-Pioneers_1/
├── index.html                  ← landing page for GitHub Pages
├── README.md
├── requirements.txt
├── LICENSE
├── .gitignore
├── ai-pipeline/
│   ├── Smart_Agarbatti_FINAL_Project_Pipeline_no_conveyor.ipynb
│   └── Smart_Agarbatti_NoComments_Pipeline.ipynb
├── dashboard/
│   ├── smart-agarbatti-dashboard.html
│   └── smart-agarbatti-dashboard.jsx
├── simulation/
│   └── working-model.html      ← the 3D working-model simulation
├── firmware/
│   └── smart_drying_chamber.ino
├── docs/
│   └── SIH2026-IDEA-Presentation-Format.pdf
└── assets/
```

---

# PART A — HARDWARE

Physical prototype: solar power, ESP32 sensing and a controlled drying chamber.

## A.1 Block Diagram

```
Solar Panel (10–20 W, charging demo)
  ↓
Battery
  ↓
ESP32
 ├── Input: DHT22 Temp/RH (GPIO 4), START (32) / STOP (33) / RESET (23)
 ├── Display: OLED SSD1306 (SDA 21 / SCL 22)
 └── Output: Heater relay (25), Circulation fan (26), Exhaust fan (27),
             LEDs R (16) / Y (17) / G (18), Buzzer (19)
  ↓
Drying Chamber (insulated box, heater, fans, sliding trolleys)
  ↓
Manual placement under camera (no conveyor)
```

## A.2 Components

| Part | Detail |
|---|---|
| ESP32 DevKit | Main controller (`firmware/smart_drying_chamber.ino`) |
| DHT22 | Temperature + humidity, GPIO 4 |
| OLED 128x64 SSD1306 | SDA 21 / SCL 22, shows T/RH, stage, drying status |
| Heater + relay module | GPIO 25, opto-isolated, fused |
| Circulation fan | GPIO 26, on during heating stages |
| Exhaust fan | GPIO 27, on when RH is high and in stages 2–3 / cooling |
| LEDs R/Y/G | GPIO 16/17/18 — red ERROR, yellow drying, green READY |
| Buzzer | GPIO 19, beeps on READY |
| Push buttons | START 32, STOP 33, RESET 23 (INPUT_PULLUP) |
| Solar panel + charge controller + battery | 10–20 W panel is a charging demo; it cannot continuously run a heater load of this size |
| Chamber box + trays/trolleys | Sliding trolleys for loading and unloading |
| Camera (USB / phone) | Fixed stand, plain contrasting background |

## A.3 Control Logic

`IDLE → STAGE1 (≥45°C) → STAGE2 (≥50°C) → STAGE3 (≥52°C and RH ≤ 35%) → COOLING (≤30°C) → READY`, plus `ERROR` above 60°C. Hysteresis ±1°C.

| State | Heater | Circ. fan | Exhaust | LED |
|---|---|---|---|---|
| IDLE | OFF | OFF | OFF | OFF |
| STAGE1 | ON | ON | ON if RH > 60% | Yellow |
| STAGE2 | ON | ON | ON | Yellow |
| STAGE3 | ON | ON | ON | Yellow |
| COOLING | OFF | ON | ON | Yellow |
| READY | OFF | OFF | OFF | Green + beep |
| ERROR | OFF | ON | ON | Red + buzzer |

Upload via Arduino IDE with ESP32 board support plus the DHT sensor, Adafruit SSD1306 and Adafruit GFX libraries. Serial at 115200 prints temperature, humidity, stage and drying status (`READY_TO_INSPECT` / `UNDER_DRYING` / `UNKNOWN`) every 2 seconds — this status string feeds the software pipeline.

## A.4 Safety

Relay isolation, fuse and ventilation are mandatory. Verify the over-temperature cutoff (>60°C → ERROR) before any demo with the heater connected.

---

# PART B — SOFTWARE

AI inspection: camera image → YOLOv8 → OpenCV → rule-based grade.

## B.1 Pipeline

```
Camera image (manual placement)
  ↓
YOLOv8 — Ready / Broken / Bent (+ box + confidence)
  ↓
OpenCV on each YOLO region — width/height, aspect ratio, colour (BGR/HSV), brightness
  ↓
ESP32 drying status (serial/Wi-Fi from Part A)
  ↓
Rule-based decision engine (plain Python rules)
  ↓
GRADE A (accept) / GRADE C (keep drying or downgrade) / REJECT (remove) / MANUAL CHECK
```

Measurement discipline followed throughout: OpenCV works on the original resolution, not a resized copy; millimetre values are reported only after camera calibration with a known reference (`PIXELS_PER_MM`), otherwise pixels.

## B.2 AI Notebook

`ai-pipeline/Smart_Agarbatti_FINAL_Project_Pipeline_no_conveyor.ipynb` is the main pipeline; `Smart_Agarbatti_NoComments_Pipeline.ipynb` is the clean demo copy.

Setup: `pip install -r requirements.txt` (ultralytics, opencv-python, matplotlib, pandas, pyyaml, numpy).

Dataset with exact class order `0 = Ready, 1 = Broken, 2 = Bent`:

```
Agarbatti_Dataset/
├── train/images/ + train/labels/
├── val/images/ + val/labels/
└── data.yaml
```

Training (YOLOv8n starting point):

```python
from ultralytics import YOLO
model = YOLO("yolov8n.pt")
model.train(data="data.yaml", epochs=50, imgsz=640, batch=8,
            project="agarbatti_runs", name="quality_grading", patience=15)
# → agarbatti_runs/quality_grading/weights/best.pt
```

Inspection run:

```python
MODEL_PATH = "agarbatti_runs/quality_grading/weights/best.pt"
IMAGE_PATH = "agarbatti_image.jpg"
DRYING_STATUS = "READY_TO_INSPECT"  # or UNDER_DRYING / UNKNOWN
results, final_records = complete_agarbatti_pipeline(IMAGE_PATH, model, DRYING_STATUS)
```

Each record carries class, confidence, box, size and colour features plus status, grade, action and reason. A batch summary table counts Grade A, Grade C, Reject and Manual Check.

Grading rules:

| Condition | Grade | Action |
|---|---|---|
| Broken | REJECT | REMOVE |
| Bent | REJECT | REMOVE |
| `UNDER_DRYING` | GRADE C | KEEP DRYING |
| `UNKNOWN` | MANUAL CHECK | CHECK SENSORS |
| Confidence below threshold | MANUAL CHECK | RECHECK IMAGE |
| Ready but measurements outside calibrated range | GRADE C | DOWNGRADE / CHECK |
| Ready, drying complete, all checks pass | GRADE A | ACCEPT |

Dimensional and brightness limits are set from real good samples; pixel-to-mm conversion requires a fixed camera position and a ruler/ArUco reference at the inspection plane.

## B.3 Dashboard

`dashboard/smart-agarbatti-dashboard.html` runs directly in the browser (CDN libraries, internet required); source in `smart-agarbatti-dashboard.jsx`. The `qualityDecision()` function ports the notebook decision rules to JS, and in-browser measurement (greyscale, threshold, bounding box, brightness) mirrors the notebook measurement step. To use the trained model, `mockYoloClassify()` is replaced with a call to an inspection backend:

```js
const form = new FormData();
form.append("image", imageBlob);
const res = await fetch("/api/inspect", { method: "POST", body: form });
const data = await res.json();
return { class: data.prediction, confidence: data.confidence };
```

Features: image upload, confidence/dimension/brightness and temperature-humidity thresholds, batch statistics with charts, per-piece checklist with reason, history view.

---

# PART C — SIMULATION

Live page: https://dayarchive-del.github.io/Digital-Pioneers_1/simulation/working-model.html
File: `simulation/working-model.html` (Three.js via CDN, opens directly in a browser).

The page is an ESP32 control-logic simulation with a 3D chamber view — it demonstrates the controller behaviour, not real heat transfer. Its own header and footer state this: "ESP32 control-logic simulation" and "Simulation of control logic only. It does not model real heat transfer or moisture loss."

## C.1 What the Page Contains

- 3D viewport with Normal / Exploded / Cutaway views, Reset cam, Labels toggle; drag to rotate, scroll to zoom, click a part for its label.
- Status panel: stage indicator, live Temperature, Humidity and Elapsed readings, heater/fan/LED chips, temperature-humidity chart.
- Controls panel: START / STOP / RESET buttons, trolley slide button, time scale 1x / 5x / 20x.
- Manual sensor override: Auto physics vs Manual mode with temperature (0–80°C) and humidity (0–100%) sliders.
- Solar and battery panel: sunlight slider, battery percentage, with the note that a 10–20 W panel is a charging demo and cannot continuously run a heater load of this size.
- Test runner: runs a scripted sequence with per-step results.
- Serial monitor: scrolling log of stage transitions and events.

## C.2 Demo Script (2 min)

1. Open the live link, press START, watch `IDLE → STAGE1 → STAGE2 → STAGE3 → COOLING → READY` on the stage indicator with the chart moving.
2. Switch time scale to 20x to fast-forward; drag the sunlight slider to move the battery reading.
3. Switch to Manual override, drag temperature above 60°C to force the ERROR state, then RESET.
4. Try Exploded and Cutaway views, then run the test sequence and show the serial monitor log.

## C.3 Simulation-to-Hardware Mapping

| Simulation element | Hardware counterpart |
|---|---|
| `updateController()` in page JS | `loop()` in `firmware/smart_drying_chamber.ino` |
| Manual temp/RH sliders | DHT22 readings |
| Heater/fan/LED chips | Relay and indicator GPIOs 25/26/27, 16/17/18 |
| Stage label + chart | OLED display + serial prints |
| Trolley slide button | Operator loading/unloading on READY beep |
| ERROR above 60°C | Same cutoff in firmware |

---

## Judge Demo (5 min)

1. Simulation link → START → full stage cycle to READY.
2. Dashboard link → upload Ready, Broken and Bent photos → grades with reasons and batch summary.
3. Notebook in Colab → Run All → result table, detection plot, batch summary.
4. Hardware: ESP32 with chamber (or photos/video) plus firmware serial log.

## Calibration Notes

- Train the classifier on the project dataset (`best.pt`) with a held-out test set before the final demo.
- Fix camera position and lighting; report pixels until ruler/ArUco calibration is done, then enable mm.
- Set width, length and brightness limits from measured good samples.
- Size the solar input and battery for the demo runtime given the heater load.
- Full checklist: notebook Section 21.

## Team — SIH 2026

- Presentation template: `docs/SIH2026-IDEA-Presentation-Format.pdf`

## License

MIT — see `LICENSE`.
