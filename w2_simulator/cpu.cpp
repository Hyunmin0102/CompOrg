#include "cpu.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iomanip>
// 스켈레톤 규칙
//  - 반환값이 있는 함수의 `return 0;` 등은 -Wreturn-type 경고를 막는 임시 값이다. 구현하면서 교체한다.
//  - `(void)x;`는 -Wunused-parameter 경고를 막는 임시 줄이다. 매개변수를 쓰기 시작하면 삭제한다.

Cpu::Cpu(size_t imemWords, size_t dmemWords) : regs_{}, pc_(0), imem_(imemWords,0), dmem_(dmemWords,0), programWords_(0) {
}

// 프로그램이 실행되기 전, 실행될 프로그램을 imem_으로 가져오는 함수
void Cpu::loadProgram(const std::vector<uint32_t>& words) {
    if(words.size() > imem_.size())
    {
        throw std::out_of_range("program too large: " + std::to_string(words.size()) + "words (instruction memory: " + std::to_string(imem_.size()) + "words)");
    }
    // imem_[i] <- words[i], 주소 : i*4
    for(size_t i = 0; i < words.size(); i++)
    {
        imem_[i] = words[i];
    }
    programWords_ = words.size();
    pc_ = 0;
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

//프로그램이 끝났는지 확인하는 함수
bool Cpu::done() const {
    return (pc_ >= programWords_ * 4);
}

uint32_t Cpu::reg(int i) const {
    return regs_.at(i);
}

uint32_t Cpu::pc() const {
    return pc_;
}

//loadWord : data memory로부터 값 읽어오는 함수
uint32_t Cpu::loadWord(uint32_t addr) const {
    // TODO: memIndex()로 인덱스를 구해 dmem_에서 읽는다.
    size_t index = memIndex(addr, dmem_.size());
    return dmem_[index];
}

//storeWord : data memory에 값 저장하는 함수
void Cpu::storeWord(uint32_t addr, uint32_t value) {
    // TODO: memIndex()로 인덱스를 구해 dmem_에 쓴다.
    size_t index = memIndex(addr, dmem_.size());
    dmem_[index] = value;
}

void Cpu::dumpRegs() const {
    // TODO: 32개 레지스터를 번호, 16진수, 10진수로 출력한다.
    //       10진수는 부호 있는 값(int32_t)으로 볼지 부호 없는 값으로 볼지 정한다.
    //       std::hex는 끈적(sticky)하다. 출력 후 std::dec로 되돌린다.
}

//instruction memory로부터 실행할 명령어를 읽어오기
uint32_t Cpu::fetch() const {
    size_t index = memIndex(pc_, imem_.size());
    return imem_[index];
}

//명령어 받은 후 해당 명령어 decode함.
DecodedInst Cpu::decodeInst(uint32_t raw) const {
    return decode(raw);
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

static std::string toHex(uint32_t value)
{
    std::ostringstream oss;
    oss << "0x" << std::setw(8) << std::setfill('0') << std::hex << value ;
    return oss.str();
}

//memIndex : Byte 주소를 이용해 몇 번째 워드인지 변환해주는 함수, Byte 주소인 addr 과 메모리 크기인 memWords를 이용.
size_t Cpu::memIndex(uint32_t addr, size_t memWords) const { 
    if(addr % 4 != 0)
    {
        throw std::invalid_argument("misaligned address: " + toHex(addr));
    }
    if(addr / 4 >= memWords)
    {
        throw std::out_of_range("out of range error: " + toHex(addr) + " (memory size: " + std::to_string(memWords * 4) + " bytes)");
    }
    return addr / 4;
}
