// W6: 공용 템플릿 함수
//   템플릿은 호출하는 쪽에서 T가 정해질 때 그 자리에서 코드가 만들어진다(인스턴스화, instantiation).
//   그래서 정의(본문)까지 헤더에 있어야 한다. .cpp로 옮기면 어떤 에러가 나는지 나중에 실험해 볼 것.
#pragma once

// x가 2의 거듭제곱(1, 2, 4, 8, ...)이면 true.
//   0과 음수는 false. 정수 타입(int, unsigned, uint64_t, ...)에서 쓴다.
template <typename T>
bool is_pow2(T x) {
    if (x <= 0)
    {
        return false;
    }
    if ((x & (x - 1)) == 0)
    {
        return true;
    }
    return false;
}
