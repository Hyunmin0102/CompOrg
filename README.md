# CompOrg

서울대 심재웅 교수님 연구실 랩인턴(2026.12) 준비용 저장소입니다.
ETH Zürich Onur Mutlu 교수님의 *Digital Design and Computer Architecture* (Spring 2025) 강의로 컴퓨터 구조를 공부하고,
매주 배운 개념을 C++17로 직접 구현합니다.

- 기간: 2026.09.30 – 11.30 (W0–W8)
- 강의: [DDCA Spring 2025](https://safari.ethz.ch/ddca/spring2025/doku.php?id=schedule)
- 참고 코드: [ChampSim](https://github.com/ChampSim/ChampSim)

## 구성

| 폴더 | 주차 | 내용 | 상태 |
|---|---|---|---|
| `w0_review` | W0 | C++ 복습 (참조/포인터, vector, unordered_map, 클래스), 첫 Makefile | 진행 중 |
| `mips` | W1–W3 | MIPS 명령어 디코더 → 명령어 시뮬레이터 → stall·CPI 측정 | 예정 |
| `bpred` | W4–W5 | 분기 예측기(1-bit, 2-bit, gshare) → 추상 클래스 리팩터링·파일 분리 | 예정 |
| `cache` | W6–W7 | 캐시 시뮬레이터(direct-mapped → set-associative, LRU, write-back) → L1/L2 + 프리페처 | 예정 |

주차별 완료 시점은 git 태그(`w0`, `w1`, ...)로 남깁니다.

## 빌드

- 환경: Ubuntu, g++ 13, GNU Make
- 컴파일 옵션: `-std=c++17 -Wall -Wextra`, 외부 라이브러리 없음

```bash
cd <폴더>
make
./sim
```