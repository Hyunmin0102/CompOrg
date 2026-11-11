// W5: 분기 예측기 공통 인터페이스 (추상 클래스, abstract class)
//   main과 simulate는 이 타입만 알고, 구체 예측기(1-bit, 2-bit, gshare)는 모른다.
//   새 예측기는 이 클래스를 상속하는 파일 하나 + predictor_factory.cpp 한 줄로 추가한다.
#pragma once

#include <cstdint>
#include <string>

class BranchPredictor {
public:
    // TODO: 가상 소멸자(virtual destructor)
    //   main은 std::unique_ptr<BranchPredictor>로 파생 클래스 객체를 지운다

    // TODO: predict — 순수 가상 함수(pure virtual function). W4 predict와 같은 역할

    // TODO: update — 순수 가상 함수. W4 update와 같은 역할

    // TODO: name — 순수 가상 함수. 결과 표의 열 이름("1-bit", "2-bit", "gshare")을 돌려준다
};
