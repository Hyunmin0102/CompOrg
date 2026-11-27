# W6: 캐시 시뮬레이터 (direct-mapped → set-associative, LRU, write-back)

메모리 접근 trace(`R 0x1a2b` / `W 0x1a2b`)를 캐시 하나에 흘려 보내고 hit/miss/writeback 통계를 낸다.
v1(direct-mapped)은 `assoc = 1`인 경우로 처리하고, v2에서 같은 `Cache` 클래스에 LRU와 write-back을 더한다.
W7에서 L1/L2 계층으로 확장할 때 `writebackToLower`가 하위 캐시 호출로 바뀐다.

정책: **write-back + write-allocate** (쓰기 miss도 블록을 채우고 dirty로 표시한다). 끝날 때 flush는 하지 않는다.

## 파일

| 파일 | 내용 | 담당 |
|---|---|---|
| `util.h` | `is_pow2` 템플릿 | TODO(1) |
| `cache.h` | `Line`, `Stats`, `Cache` 선언 | 완성됨 |
| `cache.cpp` | 생성자, 주소 분해, `findWay`, `chooseVictim`, `access` | TODO(2)–(6) |
| | `Stats::missRate`, `writebackToLower`(개수만 셈), `stats` | 완성됨 |
| `trace.h/.cpp` | `Access`, `parse_trace_line`, `load_trace` (W5 trace.cpp와 같은 구조) | 완성됨 |
| `main.cpp` | `parse_args` TODO(7), `print_sweep_csv` TODO(8). 실행 흐름·통계 출력은 완성 | 일부 |
| `tests.cpp` | 단계별 assert 테스트 (`./test <stage>`) | 완성됨 |
| `tracegen.cpp` | 행렬 순회 trace 생성기 | 완성됨 |
| `tests/sample.txt` | 손계산용 8줄 trace (`test_writeback` 2번과 같은 시퀀스) | 완성됨 |
| `tests/bad_line.txt` | 3번째 줄이 잘못된 trace (`load_trace` 오류 테스트) | 완성됨 |
| `tests/*_128x128*.txt` | `make traces`로 만드는 행렬 순회 trace | 생성됨 |

`tests.cpp`는 `cache.h`의 `friend struct CacheTester;`로 private 헬퍼(`offsetOf` 등)와 계산된 값(`numSets_` 등)을 직접 확인한다.
외부 인터페이스는 `access`와 `stats`뿐이다.

## 빌드와 실행

```bash
make              # sim, test, tracegen
./test v1         # 단계 하나만 테스트
make check        # ./test all
make traces       # tests/의 행렬 순회 trace 다시 만들기
make clean
```

```bash
./sim --trace=tests/row_128x128.txt --size=32768 --assoc=4 --block=64
./sim --trace=tests/col_128x128.txt --size=32768 --block=64 --sweep
./tracegen col 128 128 4 rw > my_trace.txt   # <row|col> <rows> <cols> <elemBytes> [r|w|rw] [base]
```

## TODO 구현 순서와 완료 확인

각 TODO의 위치는 파일에서 `TODO(번호)`로 찾는다. 한 단계가 끝나면 그 단계와 **앞 단계 테스트가 모두 통과**해야 다음으로 간다.

| 순서 | TODO | 위치 | 확인 |
|---|---|---|---|
| 1 | `is_pow2` 템플릿 | `util.h` | `./test pow2` |
| 2 | 생성자: 파라미터 검사·예외, numSets/offsetBits/indexBits, `lines_` 크기 | `cache.cpp` `Cache::Cache` | `./test ctor` |
| 3 | 주소 분해 `offsetOf` / `indexOf` / `tagOf` | `cache.cpp` | `./test addr` |
| 4 | `findWay`, `access`의 hit/miss 판정·통계 (v1, assoc = 1) | `cache.cpp` | `./test v1` |
| 5 | LRU: `useClock_`, `lastUse`, `chooseVictim`(`std::min_element` + 람다) | `cache.cpp` | `./test lru` |
| 6 | write-back: dirty bit, eviction 때 `writebackToLower` | `cache.cpp` `access` | `./test writeback` (= `./test v2`의 나머지 절반) |
| 7 | 명령줄 옵션 `--size= --assoc= --block= --trace=` | `main.cpp` `parse_args` | 아래 "TODO(7) 완료 확인" |
| 8 | `--sweep`: 연관도 1/2/4/8 CSV | `main.cpp` `print_sweep_csv` | 아래 "TODO(8) 완료 확인" |

마지막에 `make check`가 `all tests passed`로 끝나야 한다.

### 단계별 메모

- **TODO(4)** 에서 `chooseVictim`은 아직 비어 있어도 된다(assoc = 1이면 way는 항상 0). 단, `./test v1`의 쓰기 시퀀스는
  write-allocate(쓰기 miss도 채움)를 가정한다.
- **TODO(5)** `./test lru`의 첫 테스트는 v1과 같은 시퀀스를 2-way로 돌린다. 빈 way를 먼저 채우지 않으면 여기서 틀린다.
  두 번째 테스트는 FIFO나 "항상 way 0"으로 구현하면 4번 또는 5번 접근에서 틀리도록 짰다.
- **TODO(6)** victim의 블록 주소를 다시 만들 때 `writebackToLower`는 지금 개수만 세므로 주소가 틀려도 테스트는 통과한다.
  W7에서 L2가 이 주소를 받으므로 지금 손으로 한 번 확인해 둔다 (예: 2-way에서 tag 0x2, set 0 → `0x100`).
- assert가 실패하면 출력된 줄 번호로 `tests.cpp`를 열어 그 줄의 주석(계산 과정)과 내 값을 비교한다.
  gdb: `gdb --args ./test lru` → `run` → assert에서 멈추면 `bt`, `frame N`, `print c.lines_`.

### TODO(7) 완료 확인

1. `tests/sample.txt`를 `./test writeback`의 2번 테스트와 같은 캐시로 돌린다. 통계가 그 테스트의 `expect_stats` 값과 같아야 한다.
   ```bash
   ./sim --size=256 --assoc=2 --block=16 --trace=tests/sample.txt
   ```
2. 옵션 순서를 바꿔도 결과가 같다: `./sim --trace=tests/sample.txt --block=16 --assoc=2 --size=256`
3. 기본값: `--trace`만 주면 32768 / 1 / 64로 돈다 (출력 첫 줄 확인).
4. 다음은 모두 `error: ...` + usage를 출력하고 종료 코드 1이어야 한다 (`echo $?`).
   ```bash
   ./sim --size=abc --trace=tests/sample.txt     # 숫자 아님
   ./sim --size=12abc --trace=tests/sample.txt   # 뒤에 글자가 남음
   ./sim --size= --trace=tests/sample.txt        # 값 없음
   ./sim --foo=1 --trace=tests/sample.txt        # 모르는 옵션
   ./sim --size=300 --trace=tests/sample.txt     # 2의 거듭제곱 아님 (Cache 생성자 예외)
   ```
5. `./sim --size=-1 --trace=tests/sample.txt`는 어떻게 되는가? `std::stoul`이 음수를 어떻게 처리하는지 확인하고, 막을지 정한다.

### TODO(8) 완료 확인

1. 헤더 한 줄 + 연관도 1, 2, 4, 8 네 줄이 나온다.
   ```bash
   ./sim --size=256 --block=16 --trace=tests/sample.txt --sweep
   ```
   - assoc 2 줄의 수치가 TODO(7) 확인 1번 결과와 같다.
   - 각 줄이 `./sim --size=256 --block=16 --assoc=N --trace=tests/sample.txt`를 따로 돌린 결과와 같다 (N = 1, 4, 8도 확인).
2. 행렬 trace: `./sim --trace=tests/col_128x128.txt --sweep > col.csv` 처럼 저장해 열이 쉼표로 잘 나뉘는지 확인한다.
3. 캐시가 작아 어떤 연관도에서 생성자가 예외를 던질 때(예: `--size=64 --block=16`이면 assoc 8은 불가) 어떻게 처리했는지 여기에 적는다.

   **그 연관도 줄만 건너뛴다.** `Cache` 생성자의 `invalid_argument`를 연관도마다 잡아서, 이유를 stderr에
   `skip assoc 8: cache size 64 is not a multiple of ...`처럼 쓰고 다음 연관도로 넘어간다.
   stdout에는 헤더와 유효한 줄만 남으므로 `> col.csv`로 저장해도 CSV가 깨지지 않는다. 종료 코드는 0.

## 결과

기본 캐시(32KB, direct-mapped, block 64B)로 행렬 순회 trace 4개를 돌린 결과.

```
=== cache: size 32768 B, assoc 1, block 64 B ===
trace: tests/col_128x128.txt
accesses        16384
  reads         16384
  writes            0
hits                0
misses          16384
writebacks          0
miss rate      1.0000

=== cache: size 32768 B, assoc 1, block 64 B ===
trace: tests/row_128x128.txt
accesses        16384
  reads         16384
  writes            0
hits            15360
misses           1024
writebacks          0
miss rate      0.0625

=== cache: size 32768 B, assoc 1, block 64 B ===
trace: tests/col_128x128_rw.txt
accesses        32768
  reads         16384
  writes        16384
hits            16384
misses          16384
writebacks      15872
miss rate      0.5000

=== cache: size 32768 B, assoc 1, block 64 B ===
trace: tests/row_128x128_rw.txt
accesses        32768
  reads         16384
  writes        16384
hits            31744
misses           1024
writebacks        512
miss rate      0.0312
```

## 완료 기준

- [x] `make`가 경고 없이 통과한다 (`-std=c++17 -Wall -Wextra -g -O0`)
- [x] `make check`가 `all tests passed`로 끝난다
- [ ] TODO(7), TODO(8) 완료 확인 항목을 모두 해 본다
