// Motor 1
int N1 = 26;
int N2 = 27;

// Motor 2
int N3 = 12;
int N4 = 14;

enum StopMode { COAST, BRAKE };

class Motor {
  int in1, in2;
  StopMode stopMode;

  bool started3 = false;
  uint8_t stage3 = 0;            // 0=slow,1=fast,2=off
  unsigned long t3 = 0;

  bool running2 = false;
  unsigned long t2 = 0;

  void stop() {
    if (stopMode == BRAKE) { digitalWrite(in1, HIGH); digitalWrite(in2, HIGH); }
    else                   { digitalWrite(in1, LOW);  digitalWrite(in2, LOW);  }
  }
  void drive(uint8_t duty) { digitalWrite(in2, LOW); analogWrite(in1, duty); }

public:
  Motor(int a, int b, StopMode sm = BRAKE) : in1(a), in2(b), stopMode(sm) {}

  void begin() {
    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
    stop();
    unsigned long now = millis();
    t3 = now; t2 = now;
    started3 = false; running2 = false;
  }

  void runSlowFastOff(unsigned long slowMs, uint8_t slowDuty, unsigned long fastMs, unsigned long offMs) {
    unsigned long now = millis();
    if (!started3) { drive(255); t3 = now; stage3 = 0; started3 = true; return; }
    if (stage3 == 0) {
      if (now - t3 >= 300UL) drive(slowDuty);
      if (now - t3 >= slowMs) { stage3 = 1; t3 = now; drive(255); }
    } else if (stage3 == 1) {
      if (now - t3 >= fastMs) { analogWrite(in1, 0); stop(); stage3 = 2; t3 = now; }
    } else {
      if (now - t3 >= offMs) { started3 = false; }
    }
  }

  void runOnOff(unsigned long onMs, unsigned long offMs, uint8_t duty) {
    unsigned long now = millis();
    if (running2) {
      if (now - t2 >= onMs) { analogWrite(in1, 0); stop(); running2 = false; t2 = now; }
    } else {
      if (now - t2 >= offMs) { drive(duty); running2 = true; t2 = now; }
    }
  }
};

// Motor 1 direction flipped
Motor motor1(N2, N1, BRAKE);
// Motor 2 set to COAST (turns off without braking kick)
Motor motor2(N3, N4, COAST);

void setup() {
  motor1.begin();
  motor2.begin();
}

void loop() {
  // Motor 1: slow 20s then 100s on / 4s off
  motor1.runSlowFastOff(20000UL, 110, 40000UL, 4000UL);

  // Motor 2: on 20s and off 20s
  motor2.runOnOff(20000UL, 20000UL, 255);
}
