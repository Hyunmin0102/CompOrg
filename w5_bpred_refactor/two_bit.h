// W5: 2-bit 포화 카운터(saturating counter) 예측기
//   칸마다 0..3. 0,1 = not-taken 예측 / 2,3 = taken 예측
//   인덱스: (pc >> 2) & mask
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "branch_predictor.h"


class TwoBitPredictor: public BranchPredictor {
public:
    explicit TwoBitPredictor(unsigned indexBits);

    bool predict(uint64_t pc) const override;
    void update(uint64_t pc, bool taken) override;
    std::string name() const override;

private:
    size_t index(uint64_t pc) const;

    std::vector<uint8_t> table_;
    uint64_t             mask_;
};