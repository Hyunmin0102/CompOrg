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
