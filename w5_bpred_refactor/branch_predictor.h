// W5: 분기 예측기 공통 인터페이스 (추상 클래스, abstract class)
//   main과 simulate는 이 타입만 알고, 구체 예측기(1-bit, 2-bit, gshare)는 모른다.
//   새 예측기는 이 클래스를 상속하는 파일 하나 + predictor_factory.cpp 한 줄로 추가한다.
#pragma once

#include <cstdint>
#include <string>

class BranchPredictor {
public:
    virtual ~BranchPredictor() = default;
    
    virtual bool predict(uint64_t pc) const = 0;
    virtual void update(uint64_t pc, bool taken) = 0;
    virtual std::string name() const = 0;
};
