#include "predictor_factory.h"

#include <stdexcept>

#include "gshare.h"
#include "one_bit.h"
#include "two_bit.h"

// 등록 표의 한 행: 예측기 이름 + 그 예측기를 만드는 함수(함수 포인터)
struct PredictorEntry {
    std::string predictor_name;
    std::unique_ptr<BranchPredictor> (*make)(const PredictorConfig&);
};

// 등록된 예측기 표. 행 순서 = 결과 표의 열 순서.
//   새 예측기 추가 = 예측기 파일 하나 + 여기에 한 행.
//   각 행의 두 번째 칸은 캡처 없는 람다다. 캡처가 없어야 함수 포인터로 바뀐다.
//   "-> std::unique_ptr<BranchPredictor>"가 없으면 반환 타입이 파생 클래스의 unique_ptr로
//   추론되어 함수 포인터 타입과 맞지 않는다.
const std::vector<PredictorEntry> PREDICTOR_REGISTRY = {
    {"1-bit", [](const PredictorConfig& config) -> std::unique_ptr<BranchPredictor> {
        return std::make_unique<OneBitPredictor>(config.indexBits);
    }},
    {"2-bit", [](const PredictorConfig& config) -> std::unique_ptr<BranchPredictor> {
        return std::make_unique<TwoBitPredictor>(config.indexBits);
    }},
    {"gshare", [](const PredictorConfig& config) -> std::unique_ptr<BranchPredictor> {
        return std::make_unique<GsharePredictor>(config.indexBits, config.historyBits);
    }},
};

std::vector<std::string> predictor_names() {
    std::vector<std::string> names;
    for (const auto& entry : PREDICTOR_REGISTRY)
    {
        names.push_back(entry.predictor_name);
    }
    return names;
}

std::unique_ptr<BranchPredictor> make_predictor(const std::string& name, const PredictorConfig& config) {
    for (const auto& entry : PREDICTOR_REGISTRY)
    {
        if (entry.predictor_name == name)
        {
            return entry.make(config);
        }
    }
    throw std::invalid_argument("Unknown name: " + name);
}
