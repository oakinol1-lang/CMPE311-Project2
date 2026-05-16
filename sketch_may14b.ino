// ─────────────────────────────────────────────
//  PROJECT-ADDMOTOR
//  LED1 (pin 2) and LED2 (pin 3) blink via serial
//  Fan motor (pin 6 PWM) cycles speeds via button (pin 4)
// ─────────────────────────────────────────────

#include <Arduino.h>

// ── Pin definitions ──────────────────────────
#define LED1_PIN     2
#define LED2_PIN     3
#define MOTOR_PIN    6   // PWM pin
#define BUTTON_PIN   4   // button between pin 4 and GND

// ── PWM speed values (0–255) ─────────────────
#define SPEED_OFF    0
#define SPEED_LOW    80
#define SPEED_MED    160
#define SPEED_HIGH   255

// ── LED control struct ───────────────────────
struct LEDControl {

  int pin;
  unsigned long interval;
  unsigned long lastToggleTime;
  bool state;
};

LEDControl led1 = {LED1_PIN, 1000, 0, LOW};
LEDControl led2 = {LED2_PIN, 1000, 0, LOW};

// ── Fan speed sequence ───────────────────────
// OFF → LOW → MED → HIGH → MED → LOW → OFF

const int speedSequence[] = {
  SPEED_OFF,
  SPEED_LOW,
  SPEED_MED,
  SPEED_HIGH,
  SPEED_MED,
  SPEED_LOW,
  SPEED_OFF
};

const char* speedNames[] = {
  "OFF",
  "LOW",
  "MEDIUM",
  "HIGH",
  "MEDIUM",
  "LOW",
  "OFF"
};

const int SEQ_LEN = 7;

int speedIndex = 0;

// ── Button state machine ─────────────────────
enum ButtonState {

  IDLE,
  PRESSED,
  RELEASED
};

ButtonState buttonState = IDLE;

unsigned long pressTime = 0;

// ─────────────────────────────────────────────
void setup() {

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  pinMode(MOTOR_PIN, OUTPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  analogWrite(MOTOR_PIN, SPEED_OFF);

  Serial.begin(9600);

  Serial.println("=== LED + MOTOR CONTROL ===");
  Serial.println("Commands:");
  Serial.println("1 <time>  -> LED1 blink interval");
  Serial.println("2 <time>  -> LED2 blink interval");
  Serial.println("Example: 1 500");
  Serial.println("This sets LED1 to blink every 500 ms");
}

// ─────────────────────────────────────────────
void handleLED(LEDControl &led) {

  unsigned long currentTime = millis();

  if (led.interval > 0) {

    if (currentTime - led.lastToggleTime >= led.interval / 2) {

      led.state = !led.state;

      digitalWrite(led.pin, led.state);

      led.lastToggleTime = currentTime;
    }
  }
}

// ─────────────────────────────────────────────
// SERIAL COMMAND FORMAT:
//
// 1 500
// LED1 blinks every 500 ms
//
// 2 1000
// LED2 blinks every 1000 ms
// ─────────────────────────────────────────────
void handleSerial() {

  if (Serial.available() > 0) {

    int ledNumber = Serial.parseInt();
    int newInterval = Serial.parseInt();

    if (newInterval > 0) {

      if (ledNumber == 1) {

        led1.interval = newInterval;

        Serial.print("LED1 interval changed to ");
        Serial.print(newInterval);
        Serial.println(" ms");
      }

      else if (ledNumber == 2) {

        led2.interval = newInterval;

        Serial.print("LED2 interval changed to ");
        Serial.print(newInterval);
        Serial.println(" ms");
      }

      else {

        Serial.println("Invalid LED number");
      }
    }
  }
}

// ─────────────────────────────────────────────
void handleButton() {

  bool reading = digitalRead(BUTTON_PIN);

  switch (buttonState) {

    case IDLE:

      if (reading == LOW) {

        pressTime = millis();

        buttonState = PRESSED;
      }

      break;

    case PRESSED:

      // wait for release
      if (reading == HIGH) {

        // debounce
        if (millis() - pressTime > 20) {

          if (speedIndex < SEQ_LEN - 1) {

            speedIndex++;
          }

          else {

            speedIndex = 0;
          }

          analogWrite(MOTOR_PIN, speedSequence[speedIndex]);

          Serial.print("Fan speed: ");
          Serial.println(speedNames[speedIndex]);
        }

        buttonState = RELEASED;
      }

      break;

    case RELEASED:

      if (reading == HIGH) {

        buttonState = IDLE;
      }

      break;
  }
}

// ─────────────────────────────────────────────
void loop() {

  handleLED(led1);

  handleLED(led2);

  handleButton();

  handleSerial();
}