# W2 과제: MIPS 명령어 시뮬레이터 (10/12 – 10/18)

관련 강의: L8, L9a, L9b, L9c, L10 (ISA, 단일 사이클 마이크로아키텍처, 어셈블리 프로그래밍)

## 목표

W1 디코더로 해석한 명령어를 실제로 실행하는 기능 시뮬레이터(functional simulator)를 만든다.
명령어가 무엇을 하는지만 모델링하고, 몇 사이클 걸리는지는 모델링하지 않는다 (타이밍은 W3).
`step()` 한 번 = 명령어 하나 완료 (단일 사이클 프로세서와 같은 동작).

## 완료 기준

1부터 10까지 더하는 MIPS 프로그램이 시뮬레이터에서 정답(55)을 출력한다.
이 결과를 assert로 고정해 W3 이후 회귀 테스트로 쓴다.

## 구성 요소

1. 프로그램 로더: 텍스트 파일에서 한 줄에 16진수 명령어 하나씩 읽는다 (`#` 주석, 빈 줄 허용). W1 디코더를 재사용한다.
2. 아키텍처 상태 (Cpu 클래스의 private 멤버)
   - 레지스터 32개: `std::array<uint32_t, 32>`
   - PC: `uint32_t`
   - 명령어 메모리와 데이터 메모리: `std::vector<uint32_t>`, 분리 (L9b 단일 사이클 데이터패스와 같은 구조)
   - 메모리는 바이트 주소(byte addressing). 워드 배열 인덱스 = addr / 4, 정렬 검사 필요
3. 지원 명령어 5개

| 명령어 | 형식 | 인코딩 | 동작 |
|---|---|---|---|
| add rd, rs, rt | R | opcode 0, funct 0x20 | R[rd] ← R[rs] + R[rt]; PC ← PC+4 |
| addi rt, rs, imm | I | opcode 0x08 | R[rt] ← R[rs] + SignExt(imm); PC ← PC+4 |
| lw rt, imm(rs) | I | opcode 0x23 | R[rt] ← Mem[R[rs] + SignExt(imm)]; PC ← PC+4 |
| sw rt, imm(rs) | I | opcode 0x2B | Mem[R[rs] + SignExt(imm)] ← R[rt]; PC ← PC+4 |
| beq rs, rt, imm | I | opcode 0x04 | 같으면 PC ← PC+4 + (SignExt(imm) << 2), 아니면 PC+4 |

   - 분기 지연 슬롯(branch delay slot) 없음. add 오버플로 예외는 무시.
4. 메인 루프: 종료 조건까지 `step()` 반복
   - PC가 프로그램 끝을 넘으면 종료
   - 최대 실행 명령어 수를 넘으면 예외 (무한 루프 방지)
5. 오류 처리: 알 수 없는 opcode/funct, 정렬 안 된 주소, 메모리 범위 밖 접근 → 예외. main에서 잡아 PC와 함께 출력
6. 불변식: $zero 레지스터는 항상 0
7. 출력: 실행 후 레지스터 덤프(16진수/10진수), 결과 메모리 값

## 설계 방향

- fetch / decode / execute를 별도 멤버 함수로 나눈다. W3에서 "직전 명령어가 lw이고 지금 명령어가 그 결과를 쓰는가"(load-use stall)를 검사하는 코드를 끼워 넣기 위해서다.
- W1 디코더 코드는 수정하지 않고 재사용한다.

## 테스트 프로그램

- programs/sum1to10.txt: 내가 직접 MIPS 어셈블리를 쓰고 손으로 16진수로 어셈블한다 (L9c 복습).
- beq 오프셋은 명령어 개수 단위, 기준은 PC+4. 음수 오프셋(뒤로 가는 분기)과 음수 즉시값 lw/sw도 테스트한다.

## 이 과제로 배우는 C++ 문법

| 문법·개념 | 과제에서 쓰이는 곳 |
|---|---|
| 클래스 설계와 캡슐화 | 상태(레지스터, 메모리, PC)는 private, 동작(step 등)은 public |
| 생성자와 멤버 초기화 리스트 | `Cpu(size_t imemWords, size_t dmemWords) : imem_(imemWords, 0), ... {}` |
| const 멤버 함수 | `uint32_t reg(int i) const;` 상태를 바꾸지 않음을 컴파일러가 보장 |
| std::array vs std::vector | 레지스터는 크기 고정 → array, 메모리는 실행 시 크기 결정 → vector |
| .at()과 예외 | 범위 밖 접근 시 std::out_of_range. []는 검사 없이 정의되지 않은 동작 |
| throw / try / catch | 정렬 안 된 주소, 알 수 없는 opcode에서 throw, main에서 catch |
| 파일 입력 <fstream> | std::ifstream, std::getline으로 프로그램 파일 읽기 |
| 불변식 유지 | $zero = 0 규칙을 step() 한 곳에서 관리 |
| 함수 분해 | fetch / decode / execute 분리 |
| 부호 있는/없는 정수 섞기 | beq 목적지, lw/sw 주소에서 uint32_t와 int32_t가 섞일 때의 변환 규칙 |

## 이번 주에 쓰지 않는 것

헤더/소스 분리와 Makefile 확장은 W5 주제지만, W1 디코더가 이미 별도 파일이면 cpu.h/cpu.cpp로 나눠도 된다.
추상 클래스, 가상 함수, 스마트 포인터, 템플릿, std::optional은 쓰지 않는다 (W3 이후 주제).
