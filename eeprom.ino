#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <EEPROM.h>

// ========================================
// PIN DEFINITIONS
// ========================================
#define LED1_PIN   2
#define LED2_PIN   3
#define BUTTON_PIN 4
#define MOTOR_PIN  6

// ========================================
// EEPROM CONFIG
// ========================================
#define FRAME_SIZE 32
#define NUM_FRAMES 2

bool frameInUse[NUM_FRAMES] = {false, false};

// ========================================
// SEMAPHORES
// ========================================
SemaphoreHandle_t frameSemaphore;
SemaphoreHandle_t frameMutex;

// ========================================
// FAN SPEEDS
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
// SHARED VARIABLES
// ========================================
volatile unsigned long led1Interval = 500;
volatile unsigned long led2Interval = 1000;

int speedIndex = 0;

// ========================================
// EEPROM FUNCTIONS
// ========================================
int allocateFrame() {

  xSemaphoreTake(frameSemaphore, portMAX_DELAY);

  xSemaphoreTake(frameMutex, portMAX_DELAY);

  for (int i = 0; i < NUM_FRAMES; i++) {

    if (!frameInUse[i]) {

      frameInUse[i] = true;

      xSemaphoreGive(frameMutex);

      return i;
    }
  }

  xSemaphoreGive(frameMutex);

  return -1;
}

void releaseFrame(int frameNum) {

  xSemaphoreTake(frameMutex, portMAX_DELAY);

  frameInUse[frameNum] = false;

  xSemaphoreGive(frameMutex);

  xSemaphoreGive(frameSemaphore);
}

void writeFrame(int frameNum, const char* text) {

  int baseAddr = frameNum * FRAME_SIZE;

  for (int i = 0; i < FRAME_SIZE; i++) {

    if (text[i] == '\0') {

      EEPROM.write(baseAddr + i, 0);

      break;
    }

    EEPROM.write(baseAddr + i, text[i]);
  }
}

// ========================================
// TASK 1 - LED CONTROL
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

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ========================================
// TASK 2 - BUTTON + MOTOR + SERIAL
// ========================================
void TaskControl(void *pvParameters) {

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(MOTOR_PIN, OUTPUT);

  analogWrite(MOTOR_PIN, SPEED_OFF);

  bool lastButton = HIGH;

  Serial.println("=== PROJECT 5 READY ===");
  Serial.println("Commands:");
  Serial.println("1 <interval>");
  Serial.println("2 <interval>");
  Serial.println("Example: 1 300");

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

      vTaskDelay(200 / portTICK_PERIOD_MS);
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

          Serial.print("LED1 interval = ");
          Serial.println(intervalVal);
        }

        else if (ledNum == 2) {

          led2Interval = intervalVal;

          Serial.print("LED2 interval = ");
          Serial.println(intervalVal);
        }
      }
    }

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// ========================================
// TASK 3 - EEPROM TASK
// ========================================
void TaskEEPROM(void *pvParameters) {

  while (1) {

    int frame = allocateFrame();

    if (frame >= 0) {

      writeFrame(frame, "PROJECT5");

      Serial.println("EEPROM WRITE");

      vTaskDelay(1000 / portTICK_PERIOD_MS);

      releaseFrame(frame);
    }

    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

// ========================================
// SETUP
// ========================================
void setup() {

  Serial.begin(9600);

  // ====================================
  // CREATE SEMAPHORES
  // ====================================
  frameSemaphore =
    xSemaphoreCreateCounting(NUM_FRAMES, NUM_FRAMES);

  frameMutex =
    xSemaphoreCreateMutex();

  // ====================================
  // CREATE TASKS
  // ====================================
  xTaskCreate(
    TaskLEDs,
    "LEDs",
    100,
    NULL,
    1,
    NULL
  );

  xTaskCreate(
    TaskControl,
    "Control",
    120,
    NULL,
    1,
    NULL
  );

  xTaskCreate(
    TaskEEPROM,
    "EEPROM",
    100,
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