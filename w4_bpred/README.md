# W4: 분기 예측기 (1-bit, 2-bit, gshare)

분기 trace(PC, taken 여부)를 읽어서 세 예측기의 정확도를 비교한다.
세 예측기는 일부러 서로 독립된 클래스로 만든다(공통 기반 클래스·가상 함수 없음). W5에서 추상 클래스로 합친다.

## 파일

| 파일 | 내용 |
|---|---|
| `bpred.cpp` | 시뮬레이터. 예측기 3종, trace 로더, 결과 표 |
| `gen_trace.cpp` | 패턴별 trace 생성기 |
| `tests/*.txt` | 패턴별 테스트 trace (`make traces`로 다시 만들 수 있다) |

## 빌드와 실행

```bash
make            # bpred, gen_trace
make run        # tests/ 의 모든 trace로 정확도 비교 표
make top        # 표 + 예측기별 오예측 Top 10
make regress    # W1 디코더 assert, W2 sum1to10, W3 stall·CPI 회귀 테스트
```

```bash
./bpred [--index-bits N] [--history-bits H] [--top] <trace file>...
```

- `--index-bits N`: 예측 테이블 2^N칸 (기본 10)
- `--history-bits H`: gshare GHR 길이 (기본 = N, 범위 0..N)

## trace 형식

한 줄에 `PC(16진수) taken(0/1)`. `#` 이후는 주석, 빈 줄은 무시한다.

```
# gen_trace pattern=loop length=50 N=4
0x400120 1
0x400120 1
```

## 테스트 trace

| 파일 | 패턴 | 레코드 수 | PC |
|---|---|---|---|
| `loop_n4.txt` | T 4번, N 1번 반복 | 50 | 0x400120 |
| `loop_n9.txt` | T 9번, N 1번 반복 | 100 | 0x400120 |
| `alternate.txt` | T, N, T, N, ... | 20 | 0x400200 |
| `always_taken.txt` | 항상 T | 20 | 0x400300 |
| `always_not_taken.txt` | 항상 N | 20 | 0x400340 |
| `correlated.txt` | A(무작위) 다음 B(= A의 결과) | 2000 | A 0x400400, B 0x400404 |
| `mixed.txt` | 바깥 루프 한 번에: 안쪽 루프(T 3번, N 1번) → 번갈아 → 90% taken → 항상 N → 항상 T | 4000 | 0x400500, 0x400520, 0x400540, 0x401500, 0x400560 |

gen_trace 사용법: `./gen_trace <pattern> <length> <out file> [N]`

## 구현 순서 (권장)

1. `parseTraceLine` → `make run`의 branches 열이 레코드 수와 같고, 잘못된 줄을 넣으면 줄 번호와 함께 오류가 나는지 확인
2. `OneBitPredictor` + `simulateOneBit` → `make run`의 loop_n4 결과 확인
3. `TwoBitPredictor` + `simulateTwoBit`
4. `GsharePredictor` + `simulateGshare`
5. `topMispredicted`, `printTopMispredicted` → `make top`

## 결과

```
=== accuracy (index bits = 10, table entries = 1024, gshare history bits = 10) ===
trace                       branches     1-bit     2-bit    gshare
tests/loop_n4.txt                 50    62.00%    80.00%    96.00%
tests/loop_n9.txt                100    81.00%    90.00%    99.00%
tests/alternate.txt               20     5.00%    50.00%    75.00%
tests/always_taken.txt            20   100.00%   100.00%   100.00%
tests/always_not_taken.txt        20    95.00%    95.00%    95.00%
tests/correlated.txt            2000    48.90%    50.50%    73.35%
tests/mixed.txt                 4000    60.10%    54.70%    98.03%
```

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

## 작업 노트 (원래 docs/plan_w4.md)

### 완료 기준

- `make run`이 tests/의 모든 trace에 대해 세 예측기의 정확도 비교 표를 출력한다.
- `make top`이 예측기별 오예측 Top 10 PC를 출력한다.
- w4_bpred/README.md의 확인 과제를 손으로 계산해 시뮬레이터 결과와 비교하고, "분석" 절을 채운다.
- `make regress`(W1 디코더 assert, W2 sum1to10, W3 stall·CPI)가 계속 통과한다.
- 끝나면 `git tag w4`

### 예측기

| 예측기 | 칸의 내용 | 인덱스 |
|---|---|---|
| 1-bit | 마지막 결과 (0/1) | `(pc >> 2) & mask` |
| 2-bit | 포화 카운터 0..3 (2 이상이면 taken 예측) | `(pc >> 2) & mask` |
| gshare | 포화 카운터 0..3 + 전역 히스토리 레지스터(GHR) | `((pc >> 2) ^ ghr) & mask` |

- 예측 테이블: `std::vector<uint8_t>`, 크기 2^indexBits
- 2-bit 갱신: `std::min` / `std::max`로 포화
- 오예측: PC별로 `std::unordered_map`에 모은 뒤 vector로 옮겨 람다 + `std::sort`로 Top 10

### 진행 상황 (2026-11-11 기준 스냅샷, 진행하면서 갱신)

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

### 리뷰 방식 (이번 주 추가 규칙)

- README 확인 과제의 정답 수치(정확도, 오예측 수)는 알려 주지 않는다. 내가 손계산한 뒤 시뮬레이터와 비교한다.
- 내가 코드를 보여 주면 직접 컴파일·실행해 보고 리뷰한다.
- 리뷰 때 반복된 스타일 지적
  - `if (` / `for (` / `switch (`: 키워드 뒤에 공백
  - 지역 변수는 camelCase, 상수는 UPPER_SNAKE_CASE
  - `x += 1` 대신 `++x`, 연산자 양옆 공백
  - 구현이 끝난 함수는 TODO 주석, `(void)x;` 임시 줄, 임시 return을 지운다

### 이번 주에 쓰지 않는 것

추상 클래스, 가상 함수, 스마트 포인터, 템플릿 작성(W5 이후). 헤더/소스 분리와 Makefile 확장도 W5에서 한다.
