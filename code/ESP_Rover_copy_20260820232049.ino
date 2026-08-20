// ESP32 board version: 2.0.11
// Bluetooth Classic control

#include "BluetoothSerial.h"
#include <Arduino.h>

BluetoothSerial serialBT;

// Bluetooth signal
char btSignal;

// Initial speed
int Speed = 100;

// PWM channels
#define R 0
#define L 1

// ===============================
// YOUR MOTOR DRIVER WIRING
// ===============================

// PWM speed pins
int enA = 14;
int enB = 32;

// Motor A direction pins
int IN1 = 27;
int IN2 = 26;

// Motor B direction pins
int IN3 = 25;
int IN4 = 33;

// ===============================
// SETUP
// ===============================

void setup() {

  Serial.begin(115200);

  // Bluetooth name
  serialBT.begin("ROVER");

  // Motor pins
  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // PWM setup
  ledcSetup(R, 5000, 8);
  ledcAttachPin(enA, R);

  ledcSetup(L, 5000, 8);
  ledcAttachPin(enB, L);

  // Start stopped
  stopMotors();

  Serial.println("================================");
  Serial.println("        ROVER READY");
  Serial.println("================================");
  Serial.println("Bluetooth: ROVER");
}

// ===============================
// LOOP
// ===============================

void loop() {

  while (serialBT.available()) {

    btSignal = serialBT.read();

    Serial.print("Command: ");
    Serial.println(btSignal);

    // ===========================
    // SPEED CONTROL
    // ===========================

    if (btSignal == '0') Speed = 100;
    if (btSignal == '1') Speed = 110;
    if (btSignal == '2') Speed = 120;
    if (btSignal == '3') Speed = 130;
    if (btSignal == '4') Speed = 140;
    if (btSignal == '5') Speed = 150;
    if (btSignal == '6') Speed = 180;
    if (btSignal == '7') Speed = 200;
    if (btSignal == '8') Speed = 220;
    if (btSignal == '9') Speed = 240;
    if (btSignal == 'q') Speed = 255;

    // ===========================
    // MOVEMENT
    // ===========================

    if (btSignal == 'B') {

      backward();

    }

    else if (btSignal == 'F') {

      forward();

    }

    else if (btSignal == 'L') {

      left();

    }

    else if (btSignal == 'R') {

      right();

    }

    else if (btSignal == 'S') {

      stopMotors();

    }
  }
}

// ===============================
// FORWARD
// ===============================

void forward() {

  ledcWrite(R, Speed);
  ledcWrite(L, Speed);

  // Motor A
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // Motor B
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

// ===============================
// BACKWARD
// ===============================

void backward() {

  ledcWrite(R, Speed);
  ledcWrite(L, Speed);

  // Motor A
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  // Motor B
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

// ===============================
// LEFT
// ===============================

void left() {

  ledcWrite(R, Speed);
  ledcWrite(L, Speed);

  // Motor A forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // Motor B forward
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

// ===============================
// RIGHT
// ===============================

void right() {

  ledcWrite(R, Speed);
  ledcWrite(L, Speed);

  // Motor A backward
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  // Motor B backward
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

// ===============================
// STOP
// ===============================

void stopMotors() {

  ledcWrite(R, 0);
  ledcWrite(L, 0);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}