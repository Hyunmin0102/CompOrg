#include "gshare.h"
#include "saturating_counter.h"
#include <algorithm>

GsharePredictor::GsharePredictor(unsigned indexBits, unsigned historyBits)
    : table_(uint64_t(1) << indexBits, WEAKLY_TAKEN),
      mask_(table_.size() - 1),
      ghr_(0),
      historyMask_((uint64_t(1) << historyBits) - 1) {}

size_t GsharePredictor::index(uint64_t pc) const {
    return static_cast<size_t>(((pc >> 2) ^ ghr_) & mask_);
}

bool GsharePredictor::predict(uint64_t pc) const {
    return table_.at(index(pc)) >= TAKEN_THRESHOLD;
}

//predict 할 때는 지금까지의 값을 이용해 예측을 하고, predict 이후에는 지금 값을 포함해 update 해야함.
void GsharePredictor::update(uint64_t pc, bool taken) {
    uint8_t& counter = table_.at(index(pc));
    int count = static_cast<int>(counter);
    if (taken) {
        counter = static_cast<uint8_t>(std::min(count + 1, COUNTER_MAX));
    }
    else {
        counter = static_cast<uint8_t>(std::max(count - 1, COUNTER_MIN));
    }
    ghr_ = ((ghr_ << 1) | taken) & historyMask_;
}

std::string GsharePredictor::name() const{
    return "gshare";
}