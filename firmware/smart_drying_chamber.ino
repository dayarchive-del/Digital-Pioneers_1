// Smart Solar Agarbatti Drying Chamber — ESP32 firmware
// Team Digital Pioneers | SIH 2026
// Mirrors: simulation/Smart-Solar-Agarbatti-Drying-Chamber-Working-Model.html
//
// Wiring:
// DHT22 -> GPIO4 | OLED SDA 21 SCL 22 | Heater 25 | CircFan 26 | ExhFan 27
// LED R16 Y17 G18 | Buzzer 19 | START 32 STOP 33 RESET 23
//
// Libraries: DHT sensor library, Adafruit SSD1306, Adafruit GFX

#include <DHT.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define DHTPIN 4
#define DHTTYPE DHT22
#define OLED_SDA 21
#define OLED_SCL 22
#define PIN_HEATER 25
#define PIN_CIRC 26
#define PIN_EXH 27
#define PIN_LEDR 16
#define PIN_LEDY 17
#define PIN_LEDG 18
#define PIN_BUZZ 19
#define PIN_START 32
#define PIN_STOP 33
#define PIN_RESET 23

#define T_S1 45.0
#define T_S2 50.0
#define T_S3 52.0
#define RH_S3 35.0
#define T_COOL 30.0
#define T_ERR 60.0
#define HYST 1.0

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

enum State { IDLE, STAGE1, STAGE2, STAGE3, COOLING, READY, ERROR };
State state = IDLE;
float temp = 24, rh = 40;

void setOutputs(bool h, bool c, bool e, int led, bool buzz) {
  digitalWrite(PIN_HEATER, h);
  digitalWrite(PIN_CIRC, c);
  digitalWrite(PIN_EXH, e);
  digitalWrite(PIN_LEDR, led == 0);
  digitalWrite(PIN_LEDY, led == 1);
  digitalWrite(PIN_LEDG, led == 2);
  digitalWrite(PIN_BUZZ, buzz);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin(OLED_SDA, OLED_SCL);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(PIN_HEATER, OUTPUT); pinMode(PIN_CIRC, OUTPUT); pinMode(PIN_EXH, OUTPUT);
  pinMode(PIN_LEDR, OUTPUT); pinMode(PIN_LEDY, OUTPUT); pinMode(PIN_LEDG, OUTPUT);
  pinMode(PIN_BUZZ, OUTPUT);
  pinMode(PIN_START, INPUT_PULLUP); pinMode(PIN_STOP, INPUT_PULLUP); pinMode(PIN_RESET, INPUT_PULLUP);
  setOutputs(0,0,0,-1,0);
  Serial.println("Smart Agarbatti Chamber ready. Press START.");
}

void loop() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) temp = t;
  if (!isnan(h)) rh = h;

  if (digitalRead(PIN_START) == LOW && state == IDLE) { state = STAGE1; Serial.println("START -> STAGE1"); delay(400); }
  if (digitalRead(PIN_STOP) == LOW && state != IDLE) { state = IDLE; setOutputs(0,0,0,-1,0); Serial.println("STOP -> IDLE"); delay(400); }
  if (digitalRead(PIN_RESET) == LOW) { state = IDLE; setOutputs(0,0,0,-1,0); Serial.println("RESET -> IDLE"); delay(400); }

  switch (state) {
    case IDLE: setOutputs(0,0,0,-1,0); break;
    case STAGE1:
      setOutputs(1,1,rh > 60,1,0);
      if (temp >= T_S1 + HYST) { state = STAGE2; Serial.println("STAGE1 -> STAGE2"); }
      break;
    case STAGE2:
      setOutputs(1,1,1,1,0);
      if (temp >= T_S2 + HYST) { state = STAGE3; Serial.println("STAGE2 -> STAGE3"); }
      break;
    case STAGE3:
      setOutputs(1,1,1,1,0);
      if (temp >= T_S3 && rh <= RH_S3) { state = COOLING; Serial.println("STAGE3 -> COOLING"); }
      break;
    case COOLING:
      setOutputs(0,1,1,1,0);
      if (temp <= T_COOL - HYST) { state = READY; Serial.println("COOLING -> READY (beep)"); tone(PIN_BUZZ, 2000, 400); }
      break;
    case READY: setOutputs(0,0,0,2,0); break;
    case ERROR: setOutputs(0,1,1,0,1); break;
  }
  if (state != ERROR && temp > T_ERR) { state = ERROR; Serial.println("OVER-TEMP -> ERROR"); }

  // DRYING_STATUS for AI pipeline: READY_TO_INSPECT only in READY, UNDER_DRYING in stages, UNKNOWN in ERROR
  String drying = (state == READY) ? "READY_TO_INSPECT" : (state == ERROR) ? "UNKNOWN" : "UNDER_DRYING";

  oled.clearDisplay(); oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0,0); oled.printf("T:%.1fC RH:%.1f%%\n", temp, rh);
  oled.printf("State:%d Dry:%s\n", state, drying.c_str());
  oled.display();

  static unsigned long last = 0;
  if (millis() - last > 2000) { last = millis(); Serial.printf("T=%.1f RH=%.1f state=%d dry=%s\n", temp, rh, state, drying.c_str()); }
  delay(200);
}
