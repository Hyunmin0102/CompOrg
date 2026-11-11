// W5: 1-bit 예측기 (PC별로 마지막 결과를 기억)
//   테이블: std::vector<uint8_t>, 크기 2^indexBits, 칸마다 0(not-taken) 또는 1(taken)
//   인덱스: (pc >> 2) & mask
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "branch_predictor.h"

// TODO: OneBitPredictor 선언 (W4 bpred.cpp의 선언을 옮긴다)
//   - BranchPredictor를 상속한다: 상속 접근 지정자(public inheritance)
//   - predict / update / name: override 키워드
//   - 생성자: explicit
//   - private 멤버: W4와 같다
class OneBitPredictor {
};
