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
2. `OneBitPredictor` + `simulateOneBit` → 아래 확인 과제 1번과 비교
3. `TwoBitPredictor` + `simulateTwoBit`
4. `GsharePredictor` + `simulateGshare`
5. `topMispredicted`, `printTopMispredicted` → `make top`

## 확인 과제 (손으로 계산한 뒤 시뮬레이터 결과와 비교)

먼저 정할 것: 1-bit 테이블 초기값 = ____, 2-bit/gshare 카운터 초기값 = ____ (이유: ____).
아래 손계산은 모두 이 초기값을 기준으로 한다. 맞지 않으면 초기값을 바꿔서 다시 돌려 보고, 차이가 어디서 생기는지(첫 반복인가, 정상 상태인가) 적는다.

1. **loop(N=4)**, `loop_n4.txt` (50개 = 10회 반복)
   - 1-bit: 첫 반복의 오예측 수 ____, 그 뒤 반복 하나당 오예측 수 ____ → 정확도 ____
   - 2-bit: 같은 방식으로 ____ / ____ → ____
   - 1-bit는 루프 하나당 왜 그만큼 틀리는가? 2-bit는 어느 상태 전이 덕분에 줄어드는가?
2. **loop(N=9)**, `loop_n9.txt`: 1번과 같은 식으로 계산한다. N이 커지면 1-bit와 2-bit의 차이는 어떻게 되는가? 일반식(N에 대한 식)으로 써 본다.
3. **alternate**, `alternate.txt` (20개)
   - 1-bit, 2-bit 각각 손계산. 2-bit 결과는 카운터 초기값에 따라 달라지는가? 두 가지 초기값으로 계산해 본다.
   - gshare를 `--history-bits 1`로 돌리면 어떻게 될지 먼저 예상한 뒤 실행한다.
4. **always_taken / always_not_taken** (각 20개): 예측기마다 처음 몇 번 틀리는가? 초기값과의 관계를 설명한다.
5. **loop(N=4)에서 gshare 히스토리 길이**: `./bpred --history-bits 2 tests/loop_n4.txt`와 `--history-bits 8`을 비교한다. 루프 탈출(not-taken)을 맞히려면 히스토리가 최소 몇 비트여야 하는가? 손으로 근거를 쓰고 실행으로 확인한다.
6. **correlated**: A와 B를 따로 생각해서 예측기별 기대 정확도를 추정한다(정확한 값이 아니라 근거 있는 어림). `--top`으로 PC별 오예측 수를 보고 추정이 맞는지 확인한다.
7. **mixed**: `--top` 결과에서 1-bit/2-bit의 오예측이 많은 PC를 찾는다. mixed의 PC 중 테이블의 같은 칸으로 가는 쌍(aliasing)이 있다. 어느 쌍인지 PC 비트로 계산하고, `--index-bits`를 4, 10, 14로 바꿔 가며 정확도 변화를 설명한다.

## 결과

`make run` 출력을 붙여 넣는다.

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

## 분석

각 trace에서 세 예측기의 정확도가 왜 다른지 쓴다.

### loop (N=4, N=9)

- 1-bit vs 2-bit:
- 2-bit vs gshare:
- N이 커질 때:

### alternate

- 1-bit가 이 결과인 이유:
- 2-bit와 초기값:
- gshare:

### always_taken / always_not_taken

-

### correlated

- 1-bit/2-bit가 B를 못 맞히는 이유:
- gshare가 B를 맞힐 수 있는 이유 (GHR에 무엇이 들어 있는가):
- A는 어느 예측기로도 왜 나아지지 않는가:

### mixed

- aliasing이 생기는 PC 쌍과 계산:
- index bits에 따른 변화:
- gshare가 aliasing을 피하는(또는 새로 만드는) 이유:

### 정리: 어떤 패턴에서 어떤 예측기가 유리한가

-

### W5 리팩터링 메모: 세 클래스·simulate 함수에서 중복된 부분

-
