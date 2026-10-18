#include "cpu.h"

#include <iostream>
#include <stdexcept>

// 스켈레톤 규칙
//  - 반환값이 있는 함수의 `return 0;` 등은 -Wreturn-type 경고를 막는 임시 값이다. 구현하면서 교체한다.
//  - `(void)x;`는 -Wunused-parameter 경고를 막는 임시 줄이다. 매개변수를 쓰기 시작하면 삭제한다.

Cpu::Cpu(size_t imemWords, size_t dmemWords) {
    // TODO: 멤버 초기화 리스트(member initializer list)로 모든 멤버를 초기화한다.
    //       형태: Cpu(...) : imem_(imemWords, 0), dmem_(...), ... {}
    //       - 초기화 리스트는 본문 { } 이 실행되기 "전에" 멤버를 만든다. 본문에서 대입하는 것과 무엇이 다른가?
    //       - 멤버는 리스트에 적은 순서가 아니라 클래스에 "선언된 순서"대로 초기화된다 (-Wreorder 경고).
    // TODO(생각해 볼 것): regs_를 0으로 초기화하지 않으면 어떻게 되는가?
    //       - std::array<uint32_t, 32>는 지역 변수/멤버로 만들 때 원소를 자동으로 0으로 채우지 않는다.
    //         그 값을 읽으면 무엇이 나오는가? (정의되지 않은 동작, undefined behavior)
    //       - $zero 이외의 레지스터 초기값이 쓰레기면 sum1to10 결과가 실행할 때마다 달라질 수 있는가?
    //       - pc_와 programWords_도 마찬가지다. regs_{} 와 regs_() 는 각각 어떻게 초기화하는가?
    (void)imemWords;
    (void)dmemWords;
}

void Cpu::loadProgram(const std::vector<uint32_t>& words) {
    // TODO: 프로그램이 imem_보다 크면 예외를 던진다.
    // TODO: words를 imem_ 앞부분에 복사하고 programWords_, pc_를 설정한다.
    (void)words;
}

void Cpu::step() {
    // TODO: fetch() → decodeInst() → execute() 순서로 호출한다.
    // TODO: 실행 추적(trace) 출력. 실행할 명령어를 print(d)로 보여줄지 정한다.
    //       - 항상 출력? 아니면 멤버 플래그(예: bool trace_)로 켜고 끄기?
    //       - PC도 함께 출력하면 beq가 어디로 갔는지 확인하기 쉽다.
    // TODO: $zero 불변식(invariant). regs_[0]은 항상 0. 이 규칙을 "여기 한 곳"에서 관리한다.
    //       execute에서 rd/rt가 0인 경우를 매번 검사하는 것과 비교해 보자.
}

void Cpu::run(size_t maxInsts) {
    // TODO: done()이 될 때까지 step()을 반복한다.
    // TODO: 실행한 명령어 수가 maxInsts를 넘으면 예외를 던진다 (무한 루프 방지).
    (void)maxInsts;
}

bool Cpu::done() const {
    // TODO: PC가 프로그램 끝(programWords_ * 4)을 넘었는지 반환한다.
    return true;
}

uint32_t Cpu::reg(int i) const {
    // TODO: regs_에서 i번 레지스터를 반환한다. i가 범위 밖이면? .at()과 []의 차이를 떠올려 보자.
    (void)i;
    return 0;
}

uint32_t Cpu::pc() const {
    // TODO: pc_를 반환한다.
    return 0;
}

uint32_t Cpu::loadWord(uint32_t addr) const {
    // TODO: memIndex()로 인덱스를 구해 dmem_에서 읽는다.
    (void)addr;
    return 0;
}

void Cpu::storeWord(uint32_t addr, uint32_t value) {
    // TODO: memIndex()로 인덱스를 구해 dmem_에 쓴다.
    (void)addr;
    (void)value;
}

void Cpu::dumpRegs() const {
    // TODO: 32개 레지스터를 번호, 16진수, 10진수로 출력한다.
    //       10진수는 부호 있는 값(int32_t)으로 볼지 부호 없는 값으로 볼지 정한다.
    //       std::hex는 끈적(sticky)하다. 출력 후 std::dec로 되돌린다.
}

uint32_t Cpu::fetch() const {
    // TODO: pc_를 워드 인덱스로 바꿔 imem_에서 명령어를 읽는다 (memIndex 재사용).
    return 0;
}

DecodedInst Cpu::decodeInst(uint32_t raw) const {
    // TODO: 전역 decode()를 호출해 반환한다.
    //       W3에서 "직전 명령어가 lw이고 지금 명령어가 그 결과를 쓰는가" 검사를 끼워 넣을 자리다.
    (void)raw;
    return DecodedInst{};
}

void Cpu::execute(const DecodedInst& d) {
    // TODO: d.opcode(R형이면 d.funct)로 분기해 add / addi / lw / sw / beq를 실행한다.
    // TODO: 알 수 없는 opcode/funct면 예외를 던진다.
    // TODO: PC 갱신은 execute 끝에서 한다. 예외가 나면 pc()가 문제 명령어를 가리키게 하기 위해서다.
    // TODO(부호 섞기): lw/sw 주소 R[rs] + SignExt(imm), beq 목적지 PC+4 + (imm << 2)에서
    //       uint32_t와 int32_t를 더하면 어떤 타입으로 변환되는가? 음수 imm일 때 결과가 맞는가?
    //       음수를 왼쪽 시프트(<<)하는 것은 안전한가?
    (void)d;
}

size_t Cpu::memIndex(uint32_t addr, size_t memWords) const {
    // TODO: addr % 4 != 0 이면 정렬 오류(misaligned) 예외를 던진다.
    // TODO: addr / 4 가 memWords 이상이면 범위 밖 예외를 던진다.
    //       (.at()에 맡겨 std::out_of_range를 받는 방법과, 직접 검사해 메시지를 만드는 방법 비교)
    (void)addr;
    (void)memWords;
    return 0;
}
