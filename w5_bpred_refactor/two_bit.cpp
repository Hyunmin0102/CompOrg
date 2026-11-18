#include "two_bit.h"
#include "saturating_counter.h"
#include <algorithm>

TwoBitPredictor::TwoBitPredictor(unsigned indexBits)
    : table_(uint64_t(1)<<indexBits, WEAKLY_TAKEN), mask_(table_.size()-1) {
}

//이 함수는 1-bit index와 동일하다.
size_t TwoBitPredictor::index(uint64_t pc) const {
    return static_cast<size_t>((pc >> 2) & mask_);
}

bool TwoBitPredictor::predict(uint64_t pc) const {
    return table_.at(index(pc)) >= TAKEN_THRESHOLD;
}

void TwoBitPredictor::update(uint64_t pc, bool taken) {
    uint8_t& counter = table_.at(index(pc));
    int count = static_cast<int>(counter);
    if (taken) {
        counter = static_cast<uint8_t>(std::min(count + 1, COUNTER_MAX));
    }
    else {
        counter = static_cast<uint8_t>(std::max(count - 1, COUNTER_MIN));
    }
}

std::string TwoBitPredictor::name() const {
    return "2-bit";
}