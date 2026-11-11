#include "predictor_factory.h"

#include "gshare.h"
#include "one_bit.h"
#include "two_bit.h"

// TODO: 등록된 예측기 이름 목록을 돌려준다
//   - 확인: 새 예측기를 넣을 때 이 함수와 make_predictor 두 곳을 따로 고쳐야 하는가?
//     "한 줄 등록"이 되도록 이름과 생성 방법을 한곳에 모으는 방법을 생각해 본다
std::vector<std::string> predictor_names() {
    return {};   // 임시 반환값
}

// TODO: name에 맞는 예측기를 만들어 돌려준다
//   - 문법: std::make_unique, 파생 클래스 unique_ptr → 기반 클래스 unique_ptr 변환
//   - 확인: 모르는 이름이면? (W4 규칙: 호출자가 잘못 넘긴 값 → 어떤 예외인가)
std::unique_ptr<BranchPredictor> make_predictor(const std::string& name, const PredictorConfig& config) {
    (void)name;
    (void)config;
    return nullptr;   // 임시 반환값
}
