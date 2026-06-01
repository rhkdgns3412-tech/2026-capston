#ifndef ACTUATOR_H
#define ACTUATOR_H
#include "Arduino.h"
#include "sensors.h"

#define MOTOR_PIN 27
#define LIGHT_THRESHOLD 450.0

#define LED_RED   19
#define LED_GREEN 18
#define LED_BLUE  17

#define DFPLAYER_RX 26
#define DFPLAYER_TX 25

extern int currentDangerLevel;

void setDangerLevel(int level);
void initActuators();
void controlActuators(int dangerLevel);  
void controlLED(int dangerLevel);
void initAudio();
void playAlertForLevel(int dangerLevel);

#endif