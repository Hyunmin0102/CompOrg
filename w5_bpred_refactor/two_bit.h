// W5: 2-bit 포화 카운터(saturating counter) 예측기
//   칸마다 0..3. 0,1 = not-taken 예측 / 2,3 = taken 예측
//   인덱스: (pc >> 2) & mask
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "branch_predictor.h"

// TODO: 카운터 상수(COUNTER_MAX 등)를 어디에 둘지 정한다
//   - 2-bit와 gshare가 함께 쓴다. constexpr

// TODO: TwoBitPredictor 선언 (W4 bpred.cpp의 선언을 옮긴다)
//   - BranchPredictor를 상속, predict / update / name에 override
class TwoBitPredictor {
};
