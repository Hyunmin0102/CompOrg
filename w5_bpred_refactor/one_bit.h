// W5: 1-bit 예측기 (PC별로 마지막 결과를 기억)
//   테이블: std::vector<uint8_t>, 크기 2^indexBits, 칸마다 0(not-taken) 또는 1(taken)
//   인덱스: (pc >> 2) & mask
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "branch_predictor.h"

class OneBitPredictor : public BranchPredictor {
public:
    explicit OneBitPredictor(unsigned indexBits);

    bool predict(uint64_t pc) const override;        // true = taken으로 예측
    void update(uint64_t pc, bool taken) override;   // 실제 결과로 테이블 갱신
    std::string name() const override;

private:
    size_t index(uint64_t pc) const;

    std::vector<uint8_t> table_;
    uint64_t             mask_;
};
