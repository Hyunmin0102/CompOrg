// W6: 공용 템플릿 함수
//   템플릿은 호출하는 쪽에서 T가 정해질 때 그 자리에서 코드가 만들어진다(인스턴스화, instantiation).
//   그래서 정의(본문)까지 헤더에 있어야 한다. .cpp로 옮기면 어떤 에러가 나는지 나중에 실험해 볼 것.
#pragma once

// x가 2의 거듭제곱(1, 2, 4, 8, ...)이면 true.
//   0과 음수는 false. 정수 타입(int, unsigned, uint64_t, ...)에서 쓴다.
template <typename T>
bool is_pow2([[maybe_unused]] T x) {
    // TODO(1): 2의 거듭제곱 판정
    //   - 2의 거듭제곱을 2진수로 쓰면 1인 비트가 몇 개인가? x - 1은 2진수로 어떻게 생겼는가?
    //   - 위 두 가지를 비트 연산 하나(&)로 묶으면 반복문 없이 판정할 수 있다.
    //   - 경계 조건: x == 0, T가 int이고 x < 0인 경우 (tests: ./test pow2)
    //   - 구현이 끝나면 [[maybe_unused]]와 이 TODO 주석, 임시 return을 지운다.
    return false;
}
