# Smart Solar Agarbatti Drying Chamber
### Smart India Hackathon 2026 — Team Digital Pioneers

Solar-powered smart drying chamber for agarbatti manufacturers, combining ESP32-based process control with vision-based quality grading (YOLOv8 + OpenCV).

## Abstract

Small-scale agarbatti units still depend on open sun drying, which is inconsistent and unhygienic. Uneven moisture leads to breakage, bending and poor burning, and grading is done manually without any objective record.

This project presents a compact solar drying chamber with staged temperature control and an integrated camera inspection pipeline. Drying is managed by an ESP32 using temperature and humidity feedback. After drying, each stick is inspected under a fixed camera: YOLOv8 classifies Ready / Broken / Bent, OpenCV measures size and colour features on the detected region, and a rule-based engine combines the vision output with the dryer status to assign Grade A, Grade C or Reject. A web dashboard and a 3D working-model simulation are included for operation and demonstration.

## Problem Statement

- Open sun drying is weather dependent and exposes the product to dust, insects and rain.
- Uneven drying causes cracks, bends, fungus and inconsistent burning quality.
- Manual grading is slow, subjective and has no batch-wise data.
- Rural units need a low-cost, energy-efficient alternative that works off-grid.

## Proposed Solution

A single workflow covering drying and grading:

1. Agarbatti loaded on chamber trolleys. ESP32 runs staged heating with circulation and exhaust control.
2. OLED and serial monitor report temperature, humidity, stage and drying status.
3. Dried batch placed under the inspection camera (manual placement, no conveyor).
4. YOLOv8 detects each stick and gives class + confidence + bounding box.
5. OpenCV measures width, height, aspect ratio, colour and brightness inside the box.
6. Decision engine applies drying status + defect + measurement rules and outputs grade, action and reason.
7. Dashboard records batch results with charts and history.

## System Architecture

```
Solar Panel -> Battery -> ESP32 -> Drying Chamber -> Camera
                              DHT22 Temp/RH feedback
Camera Image -> YOLOv8 (Ready/Broken/Bent) -> OpenCV measurements
   + ESP32 Drying Status -> Decision Engine -> Grade A / Grade C / Reject
```

## Hardware Implementation

Chamber controller built around ESP32. Full sketch in `firmware/smart_drying_chamber.ino`.

**Components:** ESP32 DevKit, DHT22 temperature-humidity sensor, SSD1306 OLED display, heater with relay module, circulation fan, exhaust fan, R/Y/G indicator LEDs, buzzer, START/STOP/RESET buttons, 10-20W solar panel with charge controller and battery, insulated chamber with sliding trays, USB camera with fixed stand.

**I/O mapping:** DHT22 - GPIO 4. OLED SDA/SCL - 21/22. Heater - 25. Circulation fan - 26. Exhaust fan - 27. LEDs R/Y/G - 16/17/18. Buzzer - 19. Buttons START/STOP/RESET - 32/33/23.

**Control stages:** IDLE > STAGE1 (45C) > STAGE2 (50C) > STAGE3 (52C and RH <= 35%) > COOLING (30C) > READY. Over-temperature above 60C moves to ERROR with fans on and buzzer. Hysteresis of 1C is applied to avoid relay chatter.

Serial output at 115200 provides temperature, humidity, stage and drying status (`READY_TO_INSPECT` / `UNDER_DRYING` / `UNKNOWN`) for the inspection stage.

## Software Implementation

### Quality inspection pipeline

Notebook: `ai-pipeline/Smart_Agarbatti_FINAL_Project_Pipeline_no_conveyor.ipynb`

- YOLOv8n trained on three classes: 0 Ready, 1 Broken, 2 Bent. Dataset layout with `train/images`, `train/labels`, `val/images`, `val/labels` and `data.yaml` is documented in the notebook.
- Training configuration: 640px input, 50 epochs, batch 8 (Ultralytics). Output weights: `agarbatti_runs/quality_grading/weights/best.pt`.
- OpenCV preprocessing uses Gaussian blur + CLAHE on the L channel; colour features are extracted from the YOLO crop using an Otsu foreground mask so the background does not bias the reading.
- Size features come from the YOLO box (width/height in pixels, aspect ratio). Millimetre values are enabled after camera calibration with a known reference (`PIXELS_PER_MM`).
- `complete_agarbatti_pipeline()` returns one record per stick with class, confidence, measurements and the final decision, plus a batch summary table.

Grading rules: Broken or Bent > Reject. Under-drying status > Grade C (keep drying). Unknown sensor status or low confidence > Manual check. Ready with measurements outside the calibrated limits > Grade C. Ready with drying complete and all checks passed > Grade A.

### Inspection dashboard

Folder: `dashboard/`

Single-file build `smart-agarbatti-dashboard.html` (React + Recharts via CDN, opens directly in a browser). Source in `smart-agarbatti-dashboard.jsx`. Supports image upload, threshold settings for confidence/dimensions/brightness and temperature-humidity limits, batch statistics with charts, per-piece checklist with reason, and history view. The decision function in the dashboard follows the same rules as the notebook.

## Simulation

File: `simulation/working-model.html`

Three.js working model of the chamber with the ESP32 control logic running in the page. It is used to demonstrate stage transitions, fan/heater/LED behaviour, trolley movement, sunlight/battery indication and the serial log without running the heater.

Views: Normal, Exploded, Cutaway, part labels. Controls: START/STOP/RESET, trolley slide, time scale 1x/5x/20x, auto-physics vs manual sensor override, sunlight slider, test sequence runner and serial monitor panel.

The simulation models control logic only; heat transfer and moisture kinetics are represented in accelerated form for demonstration.

## Results

- Staged drying completes with automatic transition to COOLING and READY indication with buzzer.
- Inspection outputs a batch table with grade, action and reason for every detected stick, suitable for operator review.
- Dashboard provides batch counts for Grade A, Grade C, Reject and Manual Check with visual charts.
- End-to-end flow verified across firmware serial log, simulation panel, notebook pipeline and dashboard.

## Tech Stack

Embedded: ESP32 (Arduino), DHT22, SSD1306 OLED. AI/Vision: YOLOv8 (Ultralytics), OpenCV, Python, Pandas, Matplotlib. Frontend/Simulation: React, Recharts, Three.js, HTML/CSS/JS.

## How to Run

Firmware:
1. Open `firmware/smart_drying_chamber.ino` in Arduino IDE with ESP32 board support.
2. Install DHT sensor, Adafruit SSD1306 and Adafruit GFX libraries. Select the correct port and upload.

AI pipeline:
1. `pip install -r requirements.txt`
2. Prepare `Agarbatti_Dataset` and `data.yaml` as described in the notebook.
3. Train to obtain `best.pt`, set `MODEL_PATH` and `IMAGE_PATH`, then Run All.

Dashboard and simulation:
1. Open `dashboard/smart-agarbatti-dashboard.html` in a browser (internet required for CDN).
2. Open `simulation/working-model.html` in a browser and press START.

## Project Structure

```
ai-pipeline/   YOLOv8 + OpenCV notebooks
dashboard/     inspection dashboard (html + jsx source)
simulation/    3D working model with control-logic simulation
firmware/      ESP32 chamber controller sketch
docs/          SIH idea presentation format
assets/        screenshots
requirements.txt
LICENSE
```

## Future Scope

- Backend API linking the dashboard directly to the trained model for live inspection.
- Payload logging of temperature-humidity curves per batch for traceability.
- Solar-battery sizing trials for extended off-grid operation.
- Field trials with manufacturing units and acceptance testing on wider defect variety.

## Conclusion

The system brings drying control and quality grading into one low-cost prototype relevant to small manufacturers. Staged ESP32 control stabilises the drying process, while YOLOv8 with OpenCV measurements provides consistent, explainable grading supported by a dashboard and simulation for training and evaluation.

## License

MIT. See `LICENSE`.
