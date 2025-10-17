#include <Wire.h>
#include <Adafruit_VL53L0X.h>

// ---------- VL53L0X task (simple millis pattern) ----------
class VL53Task {
public:
  VL53Task(uint8_t sda, uint8_t scl, uint32_t interval_ms)
  : _sda(sda), _scl(scl), _interval(interval_ms), _last(0),
    _ok(false), _outOfRange(true), _lastMM(0) {}

  bool begin() {
    Wire.begin(_sda, _scl);      // ESP32: SDA=21, SCL=22
    Wire.setClock(100000);       // start at 100 kHz for reliability
    _ok = _lox.begin(0x29, false);
    if (!_ok) Serial.println(F("VL53L0X init failed (check wiring/power)"));
    return _ok;
  }

  void update() {
    if (!_ok) return;

    uint32_t now = millis();
    if (now - _last < _interval) return;  // non-blocking timing gate
    _last = now;

    VL53L0X_RangingMeasurementData_t m;
    _lox.rangingTest(&m, false);

    if (m.RangeStatus == 4) {
      _outOfRange = true;
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
  bool _ok;
  bool _outOfRange;
  uint16_t _lastMM;
};

// ---------- LED blink task (blink period maps to distance) ----------
class LEDBlinkTask {
public:
  explicit LEDBlinkTask(uint8_t pin) : _pin(pin), _periodMs(1000), _lastToggle(0), _state(false) {}

  void begin() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    _lastToggle = millis();
  }

  // 0..400 mm => 100..1000 ms period (faster when closer)
  // >=400 mm => 1000 ms, out-of-range => 1500 ms
  void setPeriodFromDistance(uint16_t mm, bool outOfRange) {
    if (outOfRange) { _periodMs = 1500; return; }
    if (mm >= 400)   { _periodMs = 1000; return; }
    _periodMs = 100 + ((uint32_t)mm * 900) / 400;  // linear map
  }

  void update() {
    uint32_t now = millis();
    if (now - _lastToggle >= (_periodMs / 2)) {   // toggle twice per period
      _lastToggle = now;
      _state = !_state;
      digitalWrite(_pin, _state ? HIGH : LOW);
    }
  }

private:
  uint8_t _pin;
  uint32_t _periodMs, _lastToggle;
  bool _state;
};

// ---------- Config ----------
static const uint8_t SDA_PIN = 21;          // I2C SDA
static const uint8_t SCL_PIN = 22;          // I2C SCL
static const uint32_t SAMPLE_MS = 100;      // ~10 Hz sensor sampling
static const uint8_t LED_PIN = 18;          // your LED pin

VL53Task ranger(SDA_PIN, SCL_PIN, SAMPLE_MS);
LEDBlinkTask blinker(LED_PIN);

void setup() {
  Serial.begin(115200);
  ranger.begin();
  blinker.begin();
}

void loop() {
  ranger.update();
  blinker.setPeriodFromDistance(ranger.lastMM(), ranger.outOfRange());
  blinker.update();
}
