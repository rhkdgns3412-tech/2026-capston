#ifndef ACTUATOR_H
#define ACTUATOR_H
#include "Arduino.h"

#include "sensors.h"

#define MOTOR_PIN 26
#define LIGHT_THRESHOLD 450.0

// LED GPIO Pins (Active Low)
#define LED_RED 19
#define LED_GREEN 18
#define LED_BLUE 17

// Light level thresholds for 3-stage LED control
#define LIGHT_STAGE1_THRESHOLD 250.0   // Stage 1: 0-250 lux (GREEN only)
#define LIGHT_STAGE2_THRESHOLD 500.0   // Stage 2: 250-500 lux (GREEN + RED)
// Stage 3: 500+ lux (RED only)

void initActuators();
void controlActuators(const SensorData &data);
void controlLED(float lux);

#endif