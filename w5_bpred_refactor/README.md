# W5: 분기 예측기 리팩터링 (추상 클래스, 파일 분리)

W4 `bpred.cpp` 하나에 있던 세 예측기(1-bit, 2-bit, gshare)를 추상 클래스(abstract class) `BranchPredictor` 아래로 합치고 파일을 나눈다.
목표는 **새 예측기를 "파일 하나 추가 + 팩토리 함수에 한 줄 등록"만으로 끼워 넣을 수 있는 구조**다.
출력은 W4와 글자 하나까지 같아야 한다.

## 파일

| 파일 | 내용 | 담당 |
|---|---|---|
| `branch_predictor.h` | 추상 클래스 `BranchPredictor` (predict, update, name, 가상 소멸자) | 내가 구현 |
| `one_bit.h/.cpp` | `OneBitPredictor` | 내가 구현 (W4에서 옮김) |
| `two_bit.h/.cpp` | `TwoBitPredictor` | 내가 구현 (W4에서 옮김) |
| `gshare.h/.cpp` | `GsharePredictor` | 내가 구현 (W4에서 옮김) |
| `predictor_factory.h/.cpp` | `PredictorConfig`, `predictor_names`, `make_predictor` | 내가 구현 |
| `trace.h/.cpp` | `BranchRecord`, `parseTraceLine`, `loadTrace` | W4에서 그대로 옮김 |
| `main.cpp` | 인자 처리, 결과 표, Top 10은 완성. `simulate`와 예측기 목록 순회는 TODO | 일부 내가 구현 |
| `compare_w4.sh` | W4 bpred와 출력 diff 비교 | 완성됨 |

trace는 `../w4_bpred/tests/`의 것을 그대로 쓴다.

## 빌드와 실행

```bash
make            # bpred (바뀐 .cpp와 그 .cpp가 include한 헤더만 다시 컴파일)
make run        # W4 trace 전체로 정확도 비교 표
make top        # 표 + 예측기별 오예측 Top 10
make compare    # W4 bpred와 trace별·설정별 출력 diff
make regress    # W1 디코더 assert, W2 sum1to10, W3 stall·CPI, W4 예측기
make clean
```

TODO가 남아 있는 동안에는 빌드는 되지만 표에 예측기 열이 없고, `make compare`는 실패한다.

## 링크 에러 실험

에러가 나면 메시지를 Claude 프로젝트 채팅에 붙여 넣고 물어볼 것. 실험이 끝나면 원래대로 되돌리고 `make clean && make`.

1. **undefined reference**: `.cpp` 하나를 링크에서 뺀다. (예측기 코드를 옮긴 뒤에 해 본다. 지금 `gshare.cpp`는 비어 있어서 빼도 에러가 없다)
   ```bash
   make clean && make EXCLUDE=gshare.cpp
   ```
   - 컴파일 단계와 링크 단계 중 어디서 멈추는가?
   - `gshare.cpp` 대신 `one_bit.cpp`, `trace.cpp`를 빼면 메시지가 어떻게 달라지는가?

2. **multiple definition**: 헤더에 함수 정의를 넣고 두 `.cpp`에서 include한다.
   - 예: `branch_predictor.h`에 본문이 있는 일반 함수 하나를 만든다 (`inline` 없이).
     이 헤더는 여러 `.cpp`가 include하고 있다.
   - 같은 함수에 `inline`을 붙이면 어떻게 되는가? `constexpr` 변수를 헤더에 둔 것은 왜 괜찮은가?

## gdb 연습

```bash
make
cd ../w4_bpred && gdb --args ../w5_bpred_refactor/bpred tests/loop_n4.txt
```

1. **가상 함수 호출**: `simulate` 안 `predict` 호출 줄에 `break` → `run` → `step`
   - 어느 클래스의 함수로 들어가는가? `continue`로 다음 예측기 차례가 되면 같은 줄에서 어디로 가는가?
   - `print pred`와 `print *pred`(또는 `print pred` 뒤 `set print object on`)는 무엇이 다르게 보이는가?
2. **팩토리**: `break make_predictor` → `run` → `next`로 끝까지 진행한 뒤 `finish`
   - 돌아온 값을 `print`하면 무엇이 보이는가? 이름이 `"gshare"`일 때 `config`의 값은 명령줄 인자와 같은가?
3. **호출 스택과 GHR**: `break GsharePredictor::update` → `run` → `bt`
   - `main`에서 여기까지 어떤 함수를 거쳐 왔는가?
   - `next`로 GHR 갱신 줄 앞뒤에서 `print ghr_`, `print/t ghr_`을 해 보면 비트가 어떻게 바뀌는가?

## ChampSim 비교 과제

ChampSim(https://github.com/ChampSim/ChampSim) 저장소의 `branch/` 폴더(예: `bimodal`)를 읽고,
분기 예측기 모듈이 구현해야 하는 함수(초기화, 예측, 결과 반영)를 찾아 내 `BranchPredictor`와 비교한다.

1. ChampSim에서 예측기 초기화는 어디서, 어떤 형태로 일어나는가? 내 구조의 생성자 + `PredictorConfig`와 비교하면 무엇이 다른가?
2. 결과 반영 함수가 받는 인자 중 내 `update(pc, taken)`에 없는 것은 무엇인가? 그 정보는 어떤 예측기에 필요한가?
3. ChampSim은 새 예측기를 어떻게 등록하고 선택하는가? 내 "팩토리 한 줄 등록"과 비교해 언제 결정되는가(빌드 때 / 실행 때)? 가상 함수를 쓰는가?

### 답

(여기에 쓴다)

## 완료 기준

- [ ] `make`가 경고 없이 통과한다 (`-std=c++17 -Wall -Wextra -g -O0`)
- [ ] `make compare`가 모든 trace·설정에서 W4와 출력이 같다
- [ ] 새 예측기를 **파일 하나(.h/.cpp) + `predictor_factory.cpp` 한 줄 등록**만으로 추가할 수 있다
      (예: always-taken 예측기를 만들어 표에 열이 하나 늘어나는지 확인한 뒤 지운다)
- [ ] `make regress`(W1–W4)가 계속 통과한다
- [ ] 링크 에러 실험 2개, gdb 연습 3개, ChampSim 질문 3개를 해 본다
