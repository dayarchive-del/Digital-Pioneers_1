# ✅ TODO / Still To Do (SIH 2026)

> From notebook calibration checklist (Sec 21).

## 🔴 Must-do before SIH demo
- [ ] Collect REAL dataset: 300+ photos each of Ready / Broken / Bent, varied light, angles, batches, backgrounds
- [ ] Label in Roboflow / LabelImg with exact IDs: `0=Ready, 1=Broken, 2=Bent` — verify no swapped IDs
- [ ] Train YOLOv8n (`epochs=50, imgsz=640`) → get `best.pt`, record precision/recall/mAP
- [ ] Keep separate REAL test set (never train on it), report accuracy to judges
- [ ] Replace `mockYoloClassify()` in `dashboard/*.jsx` with `/api/inspect` backend (Flask/FastAPI + ultralytics)
- [ ] Fix camera position + lighting (fixed stand, plain contrasting background) + lock exposure
- [ ] Calibrate `PIXELS_PER_MM` with ruler / ArUco marker at inspection plane — else keep `None` and report pixels only
- [ ] Measure 30+ real GOOD samples → set `MIN/MAX_WIDTH_PX, MIN/MAX_HEIGHT_PX, MIN/MAX_BRIGHTNESS` in notebook + dashboard `DEFAULT_SETTINGS`
- [ ] Wire ESP32 per pin map in README, flash `firmware/smart_drying_chamber.ino`, test DHT22 + relays + OLED + buzzer
- [ ] Connect ESP32 drying status to inspection: Serial / Bluetooth / Wi-Fi → `DRYING_STATUS` variable (currently hardcoded)
- [ ] Solar + battery load test: measure heater + fans current, prove 10–20W panel is demo-charger only, size battery for demo runtime
- [ ] Fill `docs/SIH2026-IDEA-Presentation-Format.pdf` template (problem, solution, tech, impact, budget, roadmap) + export PPT
- [ ] Record 2-min demo video (chamber + dashboard + notebook) + 5-min judge script in README

## 🟡 Should-do (stronger score)
- [ ] Add backend `app.py`: `/api/inspect` (YOLO + OpenCV + decision engine JSON), CORS for dashboard
- [ ] Add batch CSV export + confusion matrix + batch summary charts for judges
- [ ] Safety: relay isolation, fuse, over-temp cutoff test (>60°C → ERROR), ventilation test
- [ ] Cost sheet (BOM): ESP32, DHT22, heater, fans, solar, battery, camera, chamber materials
- [ ] Field test with real agarbatti maker, collect feedback quote for slides
- [ ] Rename files (remove spaces / `(2)`): e.g. `working-model.html`, move screenshot to `assets/`
- [ ] Add `assets/demo.mp4`, `assets/chamber-photo.jpg`, `assets/circuit-diagram.png`

## 🟢 Repo hygiene
- [ ] Add team members + roles to README
- [ ] Add GitHub About description + topics: `sih2026 esp32 yolov8 opencv solar agarbatti`
- [ ] Enable Issues / Projects board for task tracking
- [ ] Large files (`best.pt`, dataset) → Git LFS or Drive link, do NOT commit raw dataset

## Calibration checklist (from notebook Sec 21 — do NOT skip)
1. Replace synthetic/repeated training images with real labelled photos
2. Class IDs exactly `0=Ready, 1=Broken, 2=Bent`
3. Test on unseen images
4. Fix camera + lighting
5. Calibrate before claiming mm
6. Set limits from real good samples
7. Wire ESP32 live output to demo
8. Never claim camera measures internal moisture
9. Always show class + confidence + measurements + drying status + grade + reason
