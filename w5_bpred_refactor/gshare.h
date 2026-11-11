// W5: gshare 예측기
//   칸은 2-bit 포화 카운터. 전역 히스토리 레지스터(GHR)에 최근 historyBits개 분기 결과를 모은다.
//   인덱스: ((pc >> 2) ^ ghr) & mask
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "branch_predictor.h"

// TODO: GsharePredictor 선언 (W4 bpred.cpp의 선언을 옮긴다)
//   - BranchPredictor를 상속, predict / update / name에 override
//   - 생성자 인자가 2개다: 팩토리에서 어떻게 넘겨받는지 predictor_factory.h와 맞춘다
class GsharePredictor {
};
