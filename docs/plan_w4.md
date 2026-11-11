# W4 과제: 분기 예측기 (1-bit, 2-bit, gshare)

관련 강의: 분기 예측 (branch prediction) — 정적 예측, 1-bit/2-bit 동적 예측, 전역 히스토리(GHR), gshare

## 목표

분기 trace(PC, taken 여부)를 읽어서 1-bit, 2-bit 포화 카운터(saturating counter), gshare 세 예측기의 정확도를 비교하는 시뮬레이터를 만든다.
세 예측기는 **일부러 서로 독립된 클래스**로 만든다(공통 기반 클래스·가상 함수 금지). 다음 주(W5)에 추상 클래스로 리팩터링할 것이므로 코드 중복이 생기는 것이 의도다.

## 완료 기준

- `make run`이 tests/의 모든 trace에 대해 세 예측기의 정확도 비교 표를 출력한다.
- `make top`이 예측기별 오예측 Top 10 PC를 출력한다.
- w4_bpred/README.md의 확인 과제를 손으로 계산해 시뮬레이터 결과와 비교하고, "분석" 절을 채운다.
- `make regress`(W1 디코더 assert, W2 sum1to10, W3 stall·CPI)가 계속 통과한다.
- 끝나면 `git tag w4`

## 구성 요소

| 파일 | 내용 | 담당 |
|---|---|---|
| `w4_bpred/gen_trace.cpp` | 패턴별 trace 생성기 (loop, alternate, always_*, correlated, mixed) | 완성됨 |
| `w4_bpred/tests/*.txt` | 패턴별 테스트 trace (`make traces`로 재생성) | 완성됨 |
| `w4_bpred/bpred.cpp` | 시뮬레이터. 로더·인자 처리·결과 표는 완성, 핵심 로직은 TODO | 내가 구현 |
| `w4_bpred/README.md` | 사용법, 손계산 확인 과제, 결과·분석 빈칸 | 내가 채움 |

trace 형식: 한 줄에 `PC(16진수) taken(0/1)`, 예: `0x400120 1`. `#` 주석과 빈 줄 허용.

## 예측기

| 예측기 | 칸의 내용 | 인덱스 |
|---|---|---|
| 1-bit | 마지막 결과 (0/1) | `(pc >> 2) & mask` |
| 2-bit | 포화 카운터 0..3 (2 이상이면 taken 예측) | `(pc >> 2) & mask` |
| gshare | 포화 카운터 0..3 + 전역 히스토리 레지스터(GHR) | `((pc >> 2) ^ ghr) & mask` |

- 예측 테이블: `std::vector<uint8_t>`, 크기 2^indexBits
- 2-bit 갱신: `std::min` / `std::max`로 포화
- 오예측: PC별로 `std::unordered_map`에 모은 뒤 vector로 옮겨 람다 + `std::sort`로 Top 10

## 구현 순서

1. `parseTraceLine`
2. `OneBitPredictor` + `simulateOneBit`
3. `TwoBitPredictor` + `simulateTwoBit`
4. `GsharePredictor` + `simulateGshare`
5. `topMispredicted`, `printTopMispredicted`
6. README 확인 과제 손계산, 결과 붙여 넣기, 분석

## 진행 상황 (2026-11-11 기준 스냅샷, 진행하면서 갱신)

코드 쪽 완료 기준(`make run`, `make top`, `make regress`)은 모두 충족. 남은 것은 README와 태그.

완료 (main에 커밋됨, 마지막 커밋 897037b)
- `parseTraceLine`: `std::hex` → `std::dec`으로 읽기, `fail()` 검사, 그 뒤 따로 `std::ws` + `eof()`로 남은 글자 검사
- `OneBitPredictor` 전체: 초기값 1(taken). 이유: 루프 back edge는 마지막 한 번 빼고 taken
- `simulateOneBit`: 예측 → 비교·집계 → 틀렸을 때만 `mispredByPc[pc]` 증가 → 분기마다 update
- `TwoBitPredictor` 전체
  - 카운터 상수는 모두 `int`: `COUNTER_MIN = 0`, `COUNTER_MAX = 3`, `WEAKLY_TAKEN = 2`, `TAKEN_THRESHOLD = 2`
    (`std::min`/`std::max`에서 `int`와 `uint8_t`가 엇갈리는 컴파일 에러를 피하려고 바꿈)
  - 초기값은 `STRONGLY_TAKEN`(3)에서 `WEAKLY_TAKEN`(2)으로 바꿨다
  - `update`: `uint8_t& counter = table_.at(index(pc))`로 칸에 직접 참조를 건다(`index` 한 번). `int`로 계산한 뒤 `static_cast<uint8_t>`로 대입
- `simulateTwoBit`
- `GsharePredictor` 전체 + `simulateGshare`
  - 초깃값: 칸은 `WEAKLY_TAKEN`(2-bit와 같은 조건), `ghr_`은 0("최근 분기가 모두 not-taken", historyBits번 분기 뒤 영향 사라짐)
  - `update` 순서: **카운터 먼저, GHR 나중**. GHR을 먼저 바꾸면 predict 때 본 칸과 다른 칸을 갱신한다
  - GHR 갱신: `((ghr_ << 1) | taken) & historyMask_`. 괄호 없이 `ghr_ << 1 + taken`은 `ghr_ << (1 + taken)`(우선순위 함정)
- `topMispredicted`: 반복자 범위 생성자로 vector 생성 → 람다 + `std::sort`(오예측 수 내림차순, 동점은 PC) → `resize(std::min(k, vec.size()))`
- `printTopMispredicted`: `copyfmt`로 서식 저장·복원, PC만 `std::hex`, 오예측이 없으면 `(none)`
- README "결과" 절에 `make run` 출력 붙여 넣음

남은 일 (진도를 따라잡은 뒤, 또는 마지막 buffer 주에)
- README 확인 과제 손계산 → 시뮬레이터 결과와 비교 → "분석" 절 쓰기
- README 확인 과제 맨 위의 초기값 칸을 채운다
- 2-bit 생성자 주석 정리: 아직 "이유는 구현 이후 생각해보겠음"으로 남아 있다. alternate와 always_not_taken에서 초기값이 3, 2, 1일 때를 손계산해 보고 고친다
- 2-bit 생성자 주석의 N은 "주기"인데, README·gen_trace의 N은 "taken 횟수"다. 기호를 맞춘다. (지금 생성자 주석에는 N이 없음. 해결됐다면 이 줄을 지운다)
- 위가 끝나면 `git tag w4`

## 리뷰 방식 (이번 주 추가 규칙)

- README 확인 과제의 정답 수치(정확도, 오예측 수)는 알려 주지 않는다. 내가 손계산한 뒤 시뮬레이터와 비교한다.
- 내가 코드를 보여 주면 직접 컴파일·실행해 보고 리뷰한다.
- 리뷰 때 반복된 스타일 지적
  - `if (` / `for (` / `switch (`: 키워드 뒤에 공백
  - 지역 변수는 camelCase, 상수는 UPPER_SNAKE_CASE
  - `x += 1` 대신 `++x`, 연산자 양옆 공백
  - 구현이 끝난 함수는 TODO 주석, `(void)x;` 임시 줄, 임시 return을 지운다

## 이 과제로 배우는 C++ 문법

| 문법·개념 | 과제에서 쓰이는 곳 |
|---|---|
| std::istringstream, 입력 조작자 std::hex / std::dec / std::ws | trace 한 줄 파싱 |
| 스트림 상태 (fail, eof) | 잘못된 줄 검출 |
| 멤버 초기화 리스트, 선언 순서 = 초기화 순서 | 예측기 생성자 (table_ → mask_) |
| std::vector의 (개수, 초기값) 생성자 vs {} (initializer_list) | 예측 테이블 |
| 비트 연산 (>>, &, ^, <<, \|), 64비트 시프트 | 인덱스, mask, GHR |
| 정수 승격(integral promotion), static_cast | 포화 카운터와 std::min / std::max |
| constexpr 상수 | 카운터 상한·하한·경계값·초기값 |
| 범위 기반 for, const auto& | simulate 루프 |
| std::unordered_map::operator[] | PC별 오예측 수 |
| 람다(lambda), std::sort | 오예측 Top 10 정렬 |
| 구조적 바인딩(structured binding) | `for (const auto& [pc, cnt] : ...)` |

## 지금까지 걸린 함정

- vector를 `{}`로 만들면 initializer_list 생성자가 선택된다 → (개수, 초기값)은 `()`
- `1 << n`은 int 시프트 → 64비트 1에서 시작한다
- C++의 `^`는 거듭제곱이 아니라 XOR
- `std::ws`는 이미 eof인 스트림에 failbit를 세운다 → 값 읽기의 fail 검사와 분리한다
- 예외 종류: 호출자가 잘못 넘긴 값은 `invalid_argument`, 내부 불변식이 깨진 경우는 `logic_error`나 `assert`

## 이번 주에 쓰지 않는 것

추상 클래스, 가상 함수, 스마트 포인터, 템플릿 작성(W5 이후). 헤더/소스 분리와 Makefile 확장도 W5에서 한다.
