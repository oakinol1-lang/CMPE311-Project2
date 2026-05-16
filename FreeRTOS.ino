#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <EEPROM.h>
// ========================================
// Pin Definitions
// ========================================
#define LED1_PIN   2
#define LED2_PIN   3
#define BUTTON_PIN 4
#define MOTOR_PIN  6

// ========================================
// Fan Speeds
// ========================================
#define SPEED_OFF  0
#define SPEED_LOW  80
#define SPEED_MED  160
#define SPEED_HIGH 255

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

// ========================================
// Shared Variables
// ========================================
volatile unsigned long led1Interval = 1000;
volatile unsigned long led2Interval = 1000;

int speedIndex = 0;



// ========================================
// LED Task
// ========================================


void TaskLEDs(void *pvParameters) {

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  bool led1State = LOW;
  bool led2State = LOW;

  unsigned long last1 = 0;
  unsigned long last2 = 0;

  while (1) {

    unsigned long now = millis();

    // LED1
    if (now - last1 >= led1Interval / 2) {
      last1 = now;
      led1State = !led1State;
      digitalWrite(LED1_PIN, led1State);
    }

    // LED2
    if (now - last2 >= led2Interval / 2) {
      last2 = now;
      led2State = !led2State;
      digitalWrite(LED2_PIN, led2State);
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ========================================
// Control Task
// Handles Serial + Button
// ========================================
void TaskControl(void *pvParameters) {

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(MOTOR_PIN, OUTPUT);

  analogWrite(MOTOR_PIN, SPEED_OFF);

  bool lastButton = HIGH;

  Serial.println("=== FREE RTOS READY ===");
  Serial.println("Commands:");
  Serial.println("1 <interval>");
  Serial.println("2 <interval>");
  Serial.println("Example: 1 500");

  while (1) {

    // ====================================
    // BUTTON CONTROL
    // ====================================
    bool button = digitalRead(BUTTON_PIN);

    if (lastButton == HIGH && button == LOW) {

      speedIndex++;

      if (speedIndex >= SEQ_LEN) {
        speedIndex = 0;
      }

      analogWrite(MOTOR_PIN, speedSequence[speedIndex]);

      Serial.print("Fan Speed: ");
      Serial.println(speedNames[speedIndex]);

      vTaskDelay(pdMS_TO_TICKS(200));
    }

    lastButton = button;

    // ====================================
    // SERIAL CONTROL
    // ====================================
    if (Serial.available()) {

      int ledNum = Serial.parseInt();
      int intervalVal = Serial.parseInt();

      if (intervalVal > 0) {

        if (ledNum == 1) {

          led1Interval = intervalVal;

          Serial.print("LED1 interval set to ");
          Serial.println(intervalVal);
        }

        else if (ledNum == 2) {

          led2Interval = intervalVal;

          Serial.print("LED2 interval set to ");
          Serial.println(intervalVal);
        }

        else {

          Serial.println("Invalid LED number");
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// ========================================
// SETUP
// ========================================


void setup() {

  Serial.begin(9600);
  xTaskCreate(
    TaskLEDs,
    "LEDs",
    128,
    NULL,
    1,
    NULL
  );

  xTaskCreate(
    TaskControl,
    "Control",
    512,
    NULL,
    1,
    NULL
  );

}

// ========================================
// LOOP
// ========================================
void loop() {
}