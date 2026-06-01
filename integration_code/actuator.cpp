#include "actuator.h"
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>

int currentDangerLevel = 0;

HardwareSerial SerialMP3(2);
DFRobotDFPlayerMini dfplayer;
static bool audioInitialized = false;
static int lastPlayedLevel = -1;

void setDangerLevel(int level) {
  currentDangerLevel = level;
  controlLED(currentDangerLevel);
  controlActuators(currentDangerLevel);
  playAlertForLevel(currentDangerLevel);
}

void initActuators() {
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);

  Serial.println("Actuators initialized");
  initAudio();
}

void controlActuators(int dangerLevel) {
  if (dangerLevel >= 2) {
    digitalWrite(MOTOR_PIN, HIGH);
  } else {
    digitalWrite(MOTOR_PIN, LOW);
  }
}

void controlLED(int dangerLevel) {
  switch (dangerLevel) {
    case 0: // SAFE: 전부 OFF
      digitalWrite(LED_RED,   HIGH);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_BLUE,  HIGH);
      break;
    case 1: // CAUTION: GREEN만 ON
      digitalWrite(LED_RED,   LOW);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_BLUE,  HIGH);
      break;
    case 2: // DANGER: GREEN+RED ON
      digitalWrite(LED_RED,   LOW);
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_BLUE,  HIGH);
      break;
    case 3: // EMERGENCY: RED만 ON
      digitalWrite(LED_RED,   LOW);
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_BLUE,  HIGH);
      break;
  }
}

void initAudio() {
  SerialMP3.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
  if (dfplayer.begin(SerialMP3)) {
    audioInitialized = true;
    dfplayer.volume(30);
    Serial.println("DFPlayer initialized");
  } else {
    audioInitialized = false;
    Serial.println("DFPlayer init failed");
  }
}

void playAlertForLevel(int dangerLevel) {
  if (!audioInitialized) return;
  if (dangerLevel == lastPlayedLevel) return;

  switch (dangerLevel) {
    case 1: dfplayer.play(1); break;
    case 2: dfplayer.play(2); break;
    case 3: dfplayer.play(3); break;
    default: dfplayer.stop(); break;
  }
  lastPlayedLevel = dangerLevel;
}