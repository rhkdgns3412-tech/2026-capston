#include "hr_spo2.h"
// 샘플링 간격(ms) — sensors.cpp와 동일한 값 사용
//
// 계산 요약:
// 1) DC 성분: DC = mean(signal)
// 2) AC 성분(피크-투-피크): AC = max(signal - DC) - min(signal - DC)
// 3) SpO2 근사: R = (AC_red / DC_red) / (AC_ir / DC_ir)
//    SpO2 ≈ 110 - 25 * R  (경험적 선형 근사)
// 4) HR (bpm): 피크 간격 기반
//    avgIntervalSamples = 평균 피크 간격(샘플 단위)
//    avgIntervalMs = avgIntervalSamples * SAMPLE_INTERVAL_MS
//    HR = 60000 / avgIntervalMs
//
// 유효성: DC나 AC 범위가 작으면 SpO2 신뢰도 낮음. HR은 최소 2개 이상의 피크 필요.
static const float SAMPLE_INTERVAL_MS = 40.0f;

void computeHRandSpO2(const uint32_t *ir, const uint32_t *red, int len,
                     int32_t *hr, int32_t *spo2, int8_t *validHR, int8_t *validSpO2) {
  if (!ir || !red || len <= 4) {
    if (hr) *hr = 0;
    if (spo2) *spo2 = 0;
    if (validHR) *validHR = 0;
    if (validSpO2) *validSpO2 = 0;
    return;
  }

  // DC 성분(평균) 계산
  double meanIR = 0.0, meanRed = 0.0;
  for (int i = 0; i < len; i++) {
    meanIR += (double)ir[i];
    meanRed += (double)red[i];
  }
  meanIR /= len;
  meanRed /= len;

  // AC 성분 (신호에서 DC를 뺌)
  double maxACir = -1e9, minACir = 1e9;
  double maxACred = -1e9, minACred = 1e9;
  for (int i = 0; i < len; i++) {
    double a = (double)ir[i] - meanIR;
    double b = (double)red[i] - meanRed;
    if (a > maxACir) maxACir = a;
    if (a < minACir) minACir = a;
    if (b > maxACred) maxACred = b;
    if (b < minACred) minACred = b;
  }

  double acRangeIR = maxACir - minACir;
  double acRangeRed = maxACred - minACred;

  // 심박수 계산: IR 신호에서 피크 검출
  int peakIdx[200];
  int peakCount = 0;

  // 간단한 평활화 및 문턱 기반 피크 검출
  double absMax = 0.0;
  double acIR[len];
  for (int i = 0; i < len; i++) {
    acIR[i] = (double)ir[i] - meanIR;
    if (fabs(acIR[i]) > absMax) absMax = fabs(acIR[i]);
  }

  double threshold = absMax * 0.35; // 문턱값

  for (int i = 1; i < len - 1; i++) {
    if (acIR[i] > acIR[i-1] && acIR[i] >= acIR[i+1] && acIR[i] > threshold) {
      if (peakCount < (int)(sizeof(peakIdx)/sizeof(peakIdx[0]))) {
        peakIdx[peakCount++] = i;
      }
    }
  }

  int32_t computedHR = 0;
  int8_t hrValid = 0;
  if (peakCount >= 2) {
    // 평균 간격 계산
    double sumIntervals = 0.0;
    int intervals = 0;
    for (int i = 1; i < peakCount; i++) {
      int d = peakIdx[i] - peakIdx[i-1];
      if (d > 0) {
        sumIntervals += d;
        intervals++;
      }
    }
    if (intervals > 0) {
      double avgIntervalSamples = sumIntervals / intervals;
      double intervalMs = avgIntervalSamples * SAMPLE_INTERVAL_MS;
      double bpm = 60000.0 / intervalMs;
      computedHR = (int32_t)round(bpm);
      if (computedHR >= 30 && computedHR <= 220) hrValid = 1;
    }
  }

  // SpO2 간단 추정: AC/DC 비율의 비율을 사용한 선형 근사
  int32_t computedSpO2 = 0;
  int8_t spo2Valid = 0;
  if (meanIR > 50 && meanRed > 50 && acRangeIR > 50 && acRangeRed > 50) {
    double r = (acRangeRed / meanRed) / (acRangeIR / meanIR);
    // 경험적 계수: spo2 ~= 110 - 25 * R (간단 추정)
    double spo2d = 110.0 - 25.0 * r;
    if (spo2d > 100.0) spo2d = 100.0;
    if (spo2d < 50.0) spo2d = 50.0;
    computedSpO2 = (int32_t)round(spo2d);
    spo2Valid = 1;
  }

  if (hr) *hr = computedHR;
  if (spo2) *spo2 = computedSpO2;
  if (validHR) *validHR = hrValid;
  if (validSpO2) *validSpO2 = spo2Valid;
}
