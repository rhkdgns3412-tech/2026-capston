
#include "actuator.h"

void initActuators() {
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
}

void controlActuators(const SensorData &data) {
  if (data.lux > LIGHT_THRESHOLD) {
    digitalWrite(MOTOR_PIN, HIGH);
  } else {
    digitalWrite(MOTOR_PIN, LOW);
  }
}