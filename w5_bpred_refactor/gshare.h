// W5: gshare 예측기
//   칸은 2-bit 포화 카운터. 전역 히스토리 레지스터(GHR)에 최근 historyBits개 분기 결과를 모은다.
//   인덱스: ((pc >> 2) ^ ghr) & mask
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "branch_predictor.h"

class GsharePredictor : public BranchPredictor {
public:
    GsharePredictor(unsigned indexBits, unsigned historyBits); //historyBits : 최근 몇 개의 분기를 볼 건지 여부

    bool predict(uint64_t pc) const override;
    void update(uint64_t pc, bool taken) override;   // 카운터 갱신 + GHR 갱신
    std::string name() const override;
private:
    size_t index(uint64_t pc) const;

    std::vector<uint8_t> table_;
    uint64_t             mask_;
    uint64_t             ghr_;
    uint64_t             historyMask_;   // GHR을 historyBits 비트로 자르는 mask
};


