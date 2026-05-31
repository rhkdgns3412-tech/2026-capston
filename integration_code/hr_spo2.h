#ifndef HR_SPO2_H
#define HR_SPO2_H

#include <Arduino.h>

// 간단한 HR/SpO2 계산기
// 함수: computeHRandSpO2(ir, red, len, hr, spo2, validHR, validSpO2)
// 파라미터:
// - ir, red: 센서에서 수집한 샘플 버퍼 (uint32_t 배열)
// - len: 버퍼 길이(샘플 개수)
// - hr, spo2: 계산된 심박수(bpm) 및 산소포화도(%)를 반환할 포인터
// - validHR, validSpO2: 계산 결과의 유효성 플래그(0 = 유효하지 않음, 1 = 유효)
//
// 사용된 수식 및 방법(간단 명시):
// 1) DC 성분 (평균):
//    DC_ir  = mean(ir[i])
//    DC_red = mean(red[i])
//
// 2) AC 성분 (피크-투-피크):
//    AC_ir  = max(ir - DC_ir) - min(ir - DC_ir)  (피크-투-피크)
//    AC_red = max(red - DC_red) - min(red - DC_red)
//
// 3) SpO2 추정 (경험적 선형 근사):
//    R = (AC_red / DC_red) / (AC_ir / DC_ir)
//    SpO2 ≈ 110 - 25 * R
//    (코드에서는 AC를 피크-투-피크로 사용하며, DC는 평균을 사용합니다.)
//
// 4) 심박수(HR) 계산 (피크-간격 기반):
//    샘플 간격: Ts (ms) — 프로젝트의 샘플링 간격을 사용합니다.
//    평균 피크 간격(샘플 단위): avgIntervalSamples
//    평균 간격(밀리초) = avgIntervalSamples * Ts
//    HR (bpm) = 60000 / (avgIntervalSamples * Ts)
//
// 주의 및 유효성 검사:
// - 신호 크기가 충분히 크지 않으면(DC나 AC가 작은 경우) SpO2 계산은 신뢰할 수 없습니다.
// - HR은 최소 두 개 이상의 유효한 피크가 필요합니다.
// - 본 구현은 간단한 실험적 근사이며 의료용 진단 목적이 아닙니다.

void computeHRandSpO2(const uint32_t *ir, const uint32_t *red, int len,
                     int32_t *hr, int32_t *spo2, int8_t *validHR, int8_t *validSpO2);

#endif
