# 🪔 Smart Solar Agarbatti Drying Chamber — Team Digital Pioneers

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
    └── Screenshot 2026-08-30 212415.png
```

> Source folder on author PC: `C:\Users\admin\Desktop\sih`

---

## 🖥️ 1. AI Inspection Pipeline (YOLOv8 + OpenCV)

File: `ai-pipeline/Smart_Agarbatti_FINAL_Project_Pipeline_no_conveyor.ipynb`

### Setup (Colab / Jupyter)

```bash
pip install -r requirements.txt
# = ultralytics opencv-python matplotlib pandas pyyaml
```

### Dataset — must be exactly this

```text
Agarbatti_Dataset/
├── train/
│   ├── images/
│   └── labels/
├── val/
│   ├── images/
│   └── labels/
└── data.yaml
```

`data.yaml`:
```yaml
path: Agarbatti_Dataset
train: train/images
val: val/images
names:
  0: Ready
  1: Broken
  2: Bent
```

> Do NOT train width/length/colour as YOLO classes. Those are OpenCV measurements.

### Train

```python
from ultralytics import YOLO
model = YOLO("yolov8n.pt")
model.train(
  data="data.yaml",
  epochs=50,
  imgsz=640,
  batch=8,
  project="agarbatti_runs",
  name="quality_grading",
  patience=15
)
# → agarbatti_runs/quality_grading/weights/best.pt
```

Use **real photos** of Ready/Broken/Bent under different light/positions/batches. Synthetic/repeated images alone will not generalize. Keep a separate real test set unseen during training.

### Run inspection

```python
MODEL_PATH = "agarbatti_runs/quality_grading/weights/best.pt"
IMAGE_PATH = "agarbatti_image.jpg"
DRYING_STATUS = "READY_TO_INSPECT"  # or UNDER_DRYING / UNKNOWN

results, final_records = complete_agarbatti_pipeline(IMAGE_PATH, model, DRYING_STATUS)
```

Output per stick: `class, confidence, box, width_px, height_px, aspect_ratio, width_mm/height_mm (if calibrated), mean BGR/HSV, brightness, status, grade, action, reason`

### Grading logic

| Condition | Grade | Action |
|---|---|---|
| Broken (YOLO) | REJECT | REMOVE |
| Bent (YOLO) | REJECT | REMOVE |
| `UNDER_DRYING` | GRADE C | KEEP DRYING |
| `UNKNOWN` | MANUAL CHECK | CHECK SENSORS |
| confidence < 50% | MANUAL CHECK | RECHECK IMAGE |
| Ready but width/length/brightness out of calibrated range | GRADE C | DOWNGRADE / CHECK |
| Ready + drying complete + all checks pass | GRADE A | ACCEPT |

Calibration before demo — see `TODO.md`. Keep `MIN_WIDTH_PX etc = None` until measured from real good samples. For mm: `PIXELS_PER_MM = mm_reference` (e.g. 10 mm = 120 px → 12.0).

---

## 📊 2. Dashboard (Quality Inspection UI)

Files: `dashboard/smart-agarbatti-dashboard.html` + `.jsx`

- Single-file React build via CDN (Tailwind, Recharts, Lucide). **Double-click the `.html` to open**, internet required.
- Real logic: `qualityDecision()` is a direct JS port of notebook `quality_decision()` — same grades.
- Real measurement: in-browser greyscale + threshold + bounding-box + brightness on uploaded pixels (simplified stand-in for `measure_detected_object` + `extract_colour_features`).
- Mock only: `mockYoloClassify()` stands in for `best.pt` until trained. To connect real model:

```js
const form = new FormData();
form.append("image", imageBlob);
const res = await fetch("/api/inspect", { method: "POST", body: form });
const data = await res.json();
return { class: data.prediction, confidence: data.confidence };
```

Features: upload / camera, confidence + dimension sliders, temp/RH thresholds, batch stats, pie/bar/line charts, history, export.

---

## 🧪 3. 3D Working Model Simulation (ESP32 Control Logic)

File: `simulation/Smart-Solar-Agarbatti-Drying-Chamber-Working-Model.html`

Three.js visualisation + faithful ESP32 state-machine sim (not heat-transfer physics):

- Stages: `IDLE → STAGE1 (45°C) → STAGE2 (50°C) → STAGE3 (52°C + RH≤35%) → COOLING (≤30°C) → READY` + `ERROR (>60°C)`
- Controls: START/STOP/RESET, trolley slide, time-scale 1x/5x/20x, Auto-physics / Manual sensor override, sunlight slider + battery indicator, test runner, serial monitor log
- Views: Normal / Exploded / Cutaway, part labels on hover/click
- Open directly in browser. `three.js r128` via CDN.

> Note in UI footer is intentional: “Simulation of control logic only. It does not model real heat transfer or moisture loss.” Keep this honesty for SIH judges.

---

## 🔌 4. Firmware (ESP32)

File: `firmware/smart_drying_chamber.ino`

Mirrors the simulation. Pin map:

| Function | GPIO |
|---|---|
| DHT22 (temp/RH) | 4 |
| OLED SDA / SCL | 21 / 22 |
| Heater relay | 25 |
| Circulation fan | 26 |
| Exhaust fan | 27 |
| LED Red / Yellow / Green | 16 / 17 / 18 |
| Buzzer | 19 |
| START / STOP / RESET buttons | 32 / 33 / 23 |

Thresholds: `S1 45°C, S2 50°C, S3 52°C + RH 35%, Cooling 30°C, Error 60°C, Hyst ±1°C`

Upload via Arduino IDE (ESP32 board + DHT + Adafruit SSD1306 libs). Wire relay module with flyback/opto-isolation. 10–20 W solar panel is charging-demo only — cannot continuously drive heater; size battery + heater for demo accordingly.

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

## 👥 Team Digital Pioneers — SIH 2026

- Repo: https://github.com/dayarchive-del/Digital-Pioneers_1
- Local ref: `C:\Users\admin\Desktop\sih`
- Presentation template: `docs/SIH2026-IDEA-Presentation-Format.pdf`

## 📄 License

MIT — see `LICENSE`.
