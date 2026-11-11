// W5: 예측기 팩토리 (factory)
//   이름 문자열 → 예측기 객체. main은 구체 클래스를 include하지 않고 이 헤더만 쓴다.
//   새 예측기 추가 = 예측기 파일 하나 + predictor_factory.cpp에 한 줄 등록.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "branch_predictor.h"

// 예측기 생성자에 넘길 설정. 명령줄 인자(--index-bits, --history-bits)에서 채운다.
//   예측기마다 필요한 인자가 다르므로(gshare만 historyBits) 묶어서 넘긴다.
//   새 예측기에 새 설정이 필요하면 여기에 멤버를 추가한다.
struct PredictorConfig {
    unsigned indexBits   = 10;
    unsigned historyBits = 10;
};

// 등록된 예측기 이름 목록. 결과 표의 열 순서가 된다 (W4와 같게: "1-bit", "2-bit", "gshare").
std::vector<std::string> predictor_names();

// 이름에 맞는 예측기를 새로 만들어 돌려준다.
std::unique_ptr<BranchPredictor> make_predictor(const std::string& name, const PredictorConfig& config);
