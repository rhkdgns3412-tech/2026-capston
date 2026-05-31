#ifndef ACTUATOR_H
#define ACTUATOR_H
#include "Arduino.h"

#include "sensors.h"

#define MOTOR_PIN 27
#define LIGHT_THRESHOLD 450.0

// LED GPIO Pins (Active Low)
#define LED_RED 19
#define LED_GREEN 18
#define LED_BLUE 17

// DFPlayer 설정 (ESP32 기본: Serial2 사용). 필요 시 핀을 변경하세요.
#define DFPLAYER_RX 26
#define DFPLAYER_TX 25

// 오디오 제어 함수
void initAudio();
void playAlertForLevel(int dangerLevel);

void initActuators();
void controlActuators(const SensorData &data);
void controlLED(int dangerLevel);

#endif