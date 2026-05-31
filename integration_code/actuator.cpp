
#include "actuator.h"
#include "danger.h"
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// DFPlayer objects
HardwareSerial SerialMP3(2); // Serial2
DFRobotDFPlayerMini dfplayer;
static bool audioInitialized = false;
static int lastPlayedLevel = -1;

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


   initAudio();
}

void controlActuators(const SensorData &data) {
  int dangerLevel = calculateDangerLevel(data, DEFAULT_BASELINE_TEMP, DEFAULT_BASELINE_HR);

  // DANGER일 때만 모터 작동
  if (dangerLevel == 2) {
    digitalWrite(MOTOR_PIN, HIGH);
  } else {
    digitalWrite(MOTOR_PIN, LOW);
  }
  
  // 위험도에 따른 LED 제어
  controlLED(dangerLevel);


   playAlertForLevel(dangerLevel);
}

void controlLED(int dangerLevel) {
  // 위험도 기반 LED 제어
  // LED is active LOW (LOW=ON, HIGH=OFF)

  if (dangerLevel >= 2) {
    // DANGER: RED only ON
    digitalWrite(LED_RED, LOW);      // RED off
    digitalWrite(LED_GREEN, HIGH);    // GREEN off
    digitalWrite(LED_BLUE, HIGH);     // BLUE off
  }
  else if (dangerLevel == 1) {
    // CAUTION: RED + GREEN ON
    digitalWrite(LED_RED, LOW);       // RED on
    digitalWrite(LED_GREEN, LOW);     // GREEN on
    digitalWrite(LED_BLUE, HIGH);     // BLUE off
  }
  else {
    // SAFE: GREEN only ON
    digitalWrite(LED_RED, HIGH);      // RED off
    digitalWrite(LED_GREEN, LOW);     // GREEN on
    digitalWrite(LED_BLUE, HIGH);     // BLUE off
  }
}

void initAudio() {

  SerialMP3.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
  
   if (dfplayer.begin(SerialMP3)) {
     audioInitialized = true;
     dfplayer.volume(30); // 0~30
    Serial.println("DFPlayer initialized");
   } else {
     audioInitialized = false;
     Serial.println("DFPlayer init failed - check wiring and library");
   }
}

void playAlertForLevel(int dangerLevel) {
  if (!audioInitialized) return;

  if (dangerLevel == lastPlayedLevel) return;


   if (dangerLevel == 1) {
      //CAUTION -> play track 1
     dfplayer.play(1);
     lastPlayedLevel = 1;
   }
   else if (dangerLevel == 2) {
     // DANGER -> play caution then danger in sequence (track1 then track2)
     
     // NOTE: 단순 구현으로 고정 대기 후 다음 트랙 재생
  
     dfplayer.play(2);
     lastPlayedLevel = 2;
   }
   else {
     lastPlayedLevel = 0;
   }
}