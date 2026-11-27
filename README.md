# CompOrg

서울대 심재웅 교수님 연구실 랩인턴(2026.12) 준비용 저장소입니다.
ETH Zürich Onur Mutlu 교수님의 *Digital Design and Computer Architecture* (Spring 2025) 강의로 컴퓨터 구조를 공부하고,
매주 배운 개념을 C++17로 직접 구현합니다.

스켈레톤 코드·테스트·trace 생성기는 Claude Code로 만들고, 핵심 로직은 직접 구현했습니다.
W5·W6 README의 파일 표 "담당" 열에 주어진 부분("완성됨")과 직접 구현한 부분("내가 구현", "TODO")을 구분해 두었습니다.

- 기간: 2026.09.30 – 11.30 (W0–W8)
- 강의: [DDCA Spring 2025](https://safari.ethz.ch/ddca/spring2025/doku.php?id=schedule)
- 참고 코드: [ChampSim](https://github.com/ChampSim/ChampSim)

## 구성

| 폴더 | 주차 | 내용 | 상태 |
|---|---|---|---|
| `w0_review` | W0 | C++ 복습 (참조/포인터, vector, unordered_map, 클래스), 첫 Makefile | 완료 |
| `w1_decoder` | W1 | MIPS 명령어 디코더 | 완료 |
| `w2_simulator` | W2 | MIPS 명령어 시뮬레이터 (프로그램 파일 적재·실행, 레지스터 덤프, `sum1to10` 회귀 테스트) | 완료 |
| `w3_pipeline` | W3 | 5단 파이프라인 stall·CPI 측정 (forwarding on/off, load-use stall) | 완료 |
| `w4_bpred` | W4 | 분기 예측기 1-bit, 2-bit 포화 카운터, gshare + 패턴별 trace 생성기, 오예측 Top 10 | 완료 |
| `w5_bpred_refactor` | W5 | 추상 클래스 `BranchPredictor` + 팩토리로 리팩터링, 파일 분리 (W4와 출력 비교 28/28 일치) | 완료 |
| `w6_cache` | W6 | 캐시 시뮬레이터 (direct-mapped → set-associative, LRU, write-back/write-allocate, 연관도 sweep) | 완료 |
| (예정) | W7 | L1/L2 계층 + 프리페처 | 예정 |

W2, W4–W6 폴더의 README.md에 과제 설명과 실행 결과가 있습니다.

## 빌드

- 환경: Ubuntu, g++ 13, GNU Make
- 컴파일 옵션: `-std=c++17 -Wall -Wextra`, 외부 라이브러리 없음

```bash
cd <폴더>
make
```

실행 방법은 폴더마다 다르니 각 폴더의 README.md나 Makefile을 참고하세요.