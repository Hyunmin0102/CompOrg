#include "predictor_factory.h"

#include "gshare.h"
#include "one_bit.h"
#include "two_bit.h"

#include <stdexcept>

std::vector<std::string> predictor_names() {
    return {"1-bit", "2-bit"};
}

std::unique_ptr<BranchPredictor> make_predictor(const std::string& name, const PredictorConfig& config) {
    if(name == "1-bit")
    {
        return std::make_unique<OneBitPredictor>(config.indexBits);
    }
    else if(name == "2-bit")
    {
        return std::make_unique<TwoBitPredictor>(config.indexBits);
    }
    // else if(name == "gshare")
    // {
    //     return std::make_unique<GsharePredictor>(config.indexBits, config.historyBits);
    // }
    throw std::invalid_argument("Unknown name: " + name);
}
