#include "one_bit.h"

OneBitPredictor::OneBitPredictor(unsigned indexBits)
    : table_(uint64_t{1} << indexBits, 1), mask_(table_.size() - 1) {}

size_t OneBitPredictor::index(uint64_t pc) const {
    return static_cast<size_t>((pc >> 2) & mask_);
}

bool OneBitPredictor::predict(uint64_t pc) const {
    return table_.at(index(pc)); 
}

void OneBitPredictor::update(uint64_t pc, bool taken) {
    table_.at(index(pc)) = taken;
}

std::string OneBitPredictor::name() const {
    return "1-bit";
}
