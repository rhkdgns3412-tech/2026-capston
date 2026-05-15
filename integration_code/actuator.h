#ifndef ACTUATOR_H
#define ACTUATOR_H
#include "Arduino.h"

#include "sensors.h"

#define MOTOR_PIN 26
#define LIGHT_THRESHOLD 450.0

void initActuators();
void controlActuators(const SensorData &data);

#endif