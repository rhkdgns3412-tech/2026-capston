#include "danger.h"

// 디버깅용 전처리문:
// 정의하면 조도(lux)가 2000 이상일 때 강제로 danger 레벨을 2로 설정합니다.
// 사용하려면 아래 주석을 제거하세요.
#define DEBUG_FORCE_LUX_DANGER

int calculateDangerLevel(const SensorData &data, double baselineTemp, int baselineHR) {
  // [1] 하드웨어 탈락 필터
  if (data.bodyTemp < 20.0 || data.bodyTemp > 50.0) return 0;

  // [2] 위험도 점수 계산
  int riskIndex = 0;
  double deltaTemp = data.bodyTemp - baselineTemp;
  int deltaHr = data.heartRate - baselineHR;
  double heatIndex = data.temperature + (data.humidity * 0.1);

  // HR 점수
  if (deltaHr >= 50) riskIndex += 40;
  else if (deltaHr >= 30) riskIndex += 25;
  else if (deltaHr >= 20) riskIndex += 10;

  // 체온 점수
  if (deltaTemp >= 2.0) riskIndex += 40;
  else if (deltaTemp >= 1.0) riskIndex += 25;
  else if (deltaTemp >= 0.5) riskIndex += 10;

  // 환경 점수
  if (heatIndex >= 38.0) riskIndex += 20;
  else if (heatIndex >= 35.0) riskIndex += 10;
  else if (heatIndex >= 33.0) riskIndex += 5;

  // 조도 점수
  if (data.lux >= 50000) riskIndex += 15;
  else if (data.lux >= 30000) riskIndex += 5;

  // [3] 최종 판단
  // 디버그 강제 승격: lux가 2000 이상이면 즉시 DANGER로 처리
#ifdef DEBUG_FORCE_LUX_DANGER
  if (data.lux >= 2000) return 2;
#endif
  if (riskIndex >= 60) return 2;      // DANGER
  else if (riskIndex >= 30) return 1; // CAUTION
  else return 0;                      // SAFE
}