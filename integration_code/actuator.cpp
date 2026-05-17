
#include "actuator.h"

void initActuators() {
  // Motor pin initialization
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
  
  // LED pin initialization (Active Low)
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  
  // Turn off all LEDs initially (HIGH = OFF for active low)
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
  
  Serial.println("Actuators initialized: Motor and LEDs");
}

void controlActuators(const SensorData &data) {
  // Motor control based on light threshold
  if (data.lux > LIGHT_THRESHOLD) {
    digitalWrite(MOTOR_PIN, HIGH);
  } else {
    digitalWrite(MOTOR_PIN, LOW);
  }
  
  // LED control based on light level (3 stages)
  controlLED(data.lux);
}

void controlLED(float lux) {
  // 3-stage LED control based on ambient light level
  // LED is active LOW (LOW=ON, HIGH=OFF)
  
  if (lux < LIGHT_STAGE1_THRESHOLD) {
    // Stage 1: 0-250 lux - GREEN only ON
    digitalWrite(LED_RED, HIGH);      // RED off
    digitalWrite(LED_GREEN, LOW);     // GREEN on
    digitalWrite(LED_BLUE, HIGH);     // BLUE off
    Serial.println("LED Stage 1: GREEN");
  }
  else if (lux < LIGHT_STAGE2_THRESHOLD) {
    // Stage 2: 250-500 lux - RED and GREEN ON
    digitalWrite(LED_RED, LOW);       // RED on
    digitalWrite(LED_GREEN, LOW);     // GREEN on
    digitalWrite(LED_BLUE, HIGH);     // BLUE off
    Serial.println("LED Stage 2: RED + GREEN");
  }
  else {
    // Stage 3: 500+ lux - RED only ON
    digitalWrite(LED_RED, LOW);       // RED on
    digitalWrite(LED_GREEN, HIGH);    // GREEN off
    digitalWrite(LED_BLUE, HIGH);     // BLUE off
    Serial.println("LED Stage 3: RED");
  }
}