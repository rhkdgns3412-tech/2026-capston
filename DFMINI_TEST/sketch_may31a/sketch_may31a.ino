#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>

HardwareSerial SerialMP3(2);
DFRobotDFPlayerMini dfplayer;

#define DFPLAYER_RX 26
#define DFPLAYER_TX 25

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== DFPlayer 테스트 시작 ===");

  SerialMP3.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
  delay(2000);  // SD카드 마운트 대기

  if (dfplayer.begin(SerialMP3)) {
    Serial.println("[OK] DFPlayer 초기화 성공");
    dfplayer.volume(20);  // 볼륨 0~30
  } else {
    Serial.println("[FAIL] DFPlayer 초기화 실패");
    Serial.println("  확인사항:");
    Serial.println("  1. GPIO25 ← DFPlayer TX");
    Serial.println("  2. GPIO26 → DFPlayer TX");
    Serial.println("  3. DFPlayer VCC = 5V (3.3V 안됨)");
    Serial.println("  4. GND 공통 연결 확인");
    Serial.println("  5. SD카드 FAT32 포맷 확인");
    Serial.println("  6. MP3 폴더 안에 0001.mp3 있는지 확인");
    while(true);  // 초기화 실패 시 정지
  }
}

void loop() {
  // 1번 트랙 재생
  Serial.println("[TEST] 1번 트랙 재생");
  dfplayer.play(1);
  delay(5000);  // 5초 대기 (재생 끝날 때까지)

  // 2번 트랙 재생
  Serial.println("[TEST] 2번 트랙 재생");
  dfplayer.play(2);
  delay(5000);

  // 볼륨 테스트
  Serial.println("[TEST] 볼륨 10으로 낮춤");
  dfplayer.volume(10);
  dfplayer.play(1);
  delay(5000);

  Serial.println("[TEST] 볼륨 30으로 올림");
  dfplayer.volume(30);
  dfplayer.play(1);
  delay(5000);

  // 정지
  Serial.println("[TEST] 정지");
  dfplayer.stop();
  delay(3000);
}