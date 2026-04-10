#include <Arduino.h>

#define LED1_PIN 2
#define LED2_PIN 3

// ===== LED Control Structure =====
struct LEDControl {
  int pin;
  unsigned long interval;
  unsigned long lastToggleTime;
  bool state;
};

LEDControl led1 = {LED1_PIN, 1000, 0, LOW};
LEDControl led2 = {LED2_PIN, 1000, 0, LOW};

// ===== Serial State Machine =====
enum SerialState { WAIT_LED, WAIT_INTERVAL };
SerialState currentState = WAIT_LED;
int selectedLED = 0;

// ===== Shared LED handler (non-blocking) =====
void handleLED(LEDControl &led) {
  unsigned long now = millis();
  if (led.interval > 0) {
    if (now - led.lastToggleTime >= led.interval / 2) {
      led.state = !led.state;
      digitalWrite(led.pin, led.state);
      led.lastToggleTime = now;
    }
  }
}

// ===== Task functions =====
void blinkPin2() { handleLED(led1); }
void blinkPin3() { handleLED(led2); }

void getSerialInput() {
  if (Serial.available() > 0) {
    int input = Serial.parseInt();

    if (currentState == WAIT_LED) {
      if (input == 1 || input == 2) {
        selectedLED = input;
        Serial.println("What interval (in msec)?");
        currentState = WAIT_INTERVAL;
      }
    }
    else if (currentState == WAIT_INTERVAL) {
      if (input > 0) {
        if (selectedLED == 1) led1.interval = input;
        else                  led2.interval = input;

        Serial.println("What LED? (1 or 2)");
        currentState = WAIT_LED;
      }
    }
  }
}

// ===== Cyclic Executive Task Table =====
void (*tasks[])() = {getSerialInput, blinkPin2, blinkPin3};
const int NUM_TASKS = sizeof(tasks) / sizeof(tasks[0]);

// ===== Cycle Timing =====
const unsigned long CYCLE_TIME = 10; // ms (adjust as needed)

void setup() {
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  Serial.begin(9600);
  while (!Serial);

  Serial.println("What LED? (1 or 2)");
}

// ===== Round-Robin Cyclic Executive =====
void loop() {

  unsigned long cycleStart = millis();

  // Run all tasks once (round-robin)
  for (int i = 0; i < NUM_TASKS; i++) {
    tasks[i]();
  }

  // Wait until cycle time completes
  while (millis() - cycleStart < CYCLE_TIME) {
    // idle (do nothing)
  }
}


