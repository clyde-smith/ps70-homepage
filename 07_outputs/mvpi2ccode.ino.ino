#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --------- I2C pins & cadence ----------
static const uint8_t SDA_PIN = 21;        // shared by sensor + display
static const uint8_t SCL_PIN = 22;
static const uint32_t SAMPLE_MS = 100;    // ~10 Hz

// --------- OLED setup (SSD1306 128x64 @ 0x3C) ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
static const uint8_t OLED_ADDR = 0x3C;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --------- VL53L0X task ----------
class VL53Task {
public:
  VL53Task(uint8_t sda, uint8_t scl, uint32_t interval_ms)
  : _sda(sda), _scl(scl), _interval(interval_ms), _last(0),
    _ok(false), _outOfRange(true), _lastMM(0) {}

  bool begin() {
    Wire.begin(_sda, _scl);            // ESP32 I2C pins
    Wire.setClock(100000);             // start at 100 kHz (reliable)
    _ok = _lox.begin(0x29, false);     // VL53L0X default addr
    if (!_ok) Serial.println(F("VL53L0X init failed"));
    return _ok;
  }

  void update() {
    if (!_ok) return;
    uint32_t now = millis();
    if (now - _last < _interval) return;
    _last = now;

    VL53L0X_RangingMeasurementData_t m;
    _lox.rangingTest(&m, false);

    if (m.RangeStatus == 4) {
      _outOfRange = true;
      _lastMM = 0;
      Serial.println(F("out of range"));
    } else {
      _outOfRange = false;
      _lastMM = m.RangeMilliMeter;
      Serial.print(F("Distance (mm): "));
      Serial.println(_lastMM);
    }
  }

  bool outOfRange() const { return _outOfRange; }
  uint16_t lastMM() const { return _lastMM; }

private:
  Adafruit_VL53L0X _lox;
  uint8_t _sda, _scl;
  uint32_t _interval, _last;
  bool _ok, _outOfRange;
  uint16_t _lastMM;
};

// --------- Display task ----------
class DisplayTask {
public:
  DisplayTask(uint32_t interval_ms) : _interval(interval_ms), _last(0) {}

  bool begin() {
    // Wire already begun by VL53Task
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
      Serial.println(F("SSD1306 init failed (addr 0x3C?)"));
      return false;
    }
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println(F("VL53 Ready"));
    display.display();
    return true;
  }

  void update(uint16_t mm, bool outOfRange) {
    uint32_t now = millis();
    if (now - _last < _interval) return;
    _last = now;

    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println(F("Distance"));

    display.setTextSize(3);
    display.setCursor(0, 28);
    if (outOfRange) {
      display.println(F("-- --"));
    } else {
      // Print value like "123mm"
      char buf[12];
      snprintf(buf, sizeof(buf), "%umm", (unsigned)mm);
      display.println(buf);
    }
    display.display();
  }

private:
  uint32_t _interval, _last;
};

// --------- Objects ----------
VL53Task ranger(SDA_PIN, SCL_PIN, SAMPLE_MS);
DisplayTask screen(100);  // refresh text ~10 Hz

void setup() {
  Serial.begin(115200);
  ranger.begin();        // start I2C + sensor first
  screen.begin();        // then init the OLED
}

void loop() {
  ranger.update();
  screen.update(ranger.lastMM(), ranger.outOfRange());
}
