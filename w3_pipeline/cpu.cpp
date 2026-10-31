#include "cpu.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iomanip>
#include <array>

Cpu::Cpu(size_t imemWords, size_t dmemWords) : regs_{}, pc_(0), imem_(imemWords,0), dmem_(dmemWords,0), programWords_(0), trace_(false), instCount_(0), timer_() {
}

// MIPS 레지스터 이름 (ABI 이름). 인덱스 = 레지스터 번호.
static const std::array<const char*, 32> REG_NAMES = {
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",   //  0 ~  7
    "t0",   "t1", "t2", "t3", "t4", "t5", "t6", "t7",   //  8 ~ 15
    "s0",   "s1", "s2", "s3", "s4", "s5", "s6", "s7",   // 16 ~ 23
    "t8",   "t9", "k0", "k1", "gp", "sp", "fp", "ra"    // 24 ~ 31
};

static std::string toHex(uint32_t value)
{
    std::ostringstream oss;
    oss << "0x" << std::setw(8) << std::setfill('0') << std::hex << value ;
    return oss.str();
}


void Cpu::setTrace(bool on)
{
    trace_ = on;
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

//명령어 한 사이클. 실제로 명령어를 fetch 해오고, decode하고, execute하는 함수. 디버깅을 위해 trace에 따라 명령어와 PC값을 출력할 지 여부를 결정한다.
void Cpu::step() {
    uint32_t inst = fetch();
    DecodedInst decoded_inst = decodeInst(inst);
    if(trace_)
    {
        std::cout << "[" << toHex(pc()) << "] " ;
        print(decoded_inst);
    }
    uint32_t pc_before = pc_;

    execute(decoded_inst);
    regs_[0] = 0;
    instCount_ += 1;

    bool taken = (decoded_inst.opcode == OP_BEQ) && (pc_ != pc_before + 4); //offset 0인 beq는 not-taken으로 셈.
    timer_.onInst(decoded_inst, taken);
}

// W3: 타이밍 통계
void Cpu::setForwarding(bool on) {
    timer_.setForwarding(on);
}

Stats Cpu::stats() const {
    return timer_.stats();
}

size_t Cpu::instCount() const 
{
    return instCount_;
}

//프로그램이 종료되기 전까지 명령어 사이클(fetch, decode, ... 을 반복)
void Cpu::run(size_t maxInsts) {
    while(!done()) 
    {
        if(instCount_ >= maxInsts)
        {
            std::ostringstream oss;
            oss << "maxInsts: " << maxInsts << ", pc at there: " << toHex(pc_);
            throw std::runtime_error(oss.str());
        }
        step();
    }
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
    size_t index = memIndex(addr, dmem_.size());
    return dmem_[index];
}

//storeWord : data memory에 값 저장하는 함수
void Cpu::storeWord(uint32_t addr, uint32_t value) {
    size_t index = memIndex(addr, dmem_.size());
    dmem_[index] = value;
}

// 레지스터 32개를 "이름(번호)  16진수  10진수(부호 있음)" 형식으로 출력한다.
void Cpu::dumpRegs() const {
    for (size_t i = 0; i < REG_NAMES.size(); i++)
    {
        std::cout << "$" << std::left << std::setw(4) << REG_NAMES[i] << std::right
                  << " (" << std::setw(2) << i << ")  "
                  << toHex(regs_[i]) << "  "
                  << std::setw(11) << static_cast<int32_t>(regs_[i]) << "\n";
    }
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
    uint32_t pc1 = pc_ + 4;
    if(d.opcode == OP_R)
    {
        switch (d.funct)
        {
        case FUNCT_ADD:
            {
            uint32_t value = regs_[d.rs] + regs_[d.rt];
            regs_[d.rd] = value;
            break;
            }
        default:
            throw std::invalid_argument("invalid funct: " + toHex(d.funct));
        }
    }
    else if(d.opcode == OP_ADDI)
    {
        regs_[d.rt] = regs_[d.rs] + d.imm;
    }
    else if(d.opcode == OP_LW)
    {
        uint32_t address = regs_[d.rs] + d.imm;
        regs_[d.rt] = loadWord(address);
    }
    else if(d.opcode == OP_SW)
    {
        uint32_t address = regs_[d.rs] + d.imm;
        storeWord(address, regs_[d.rt]);
    }
    else if(d.opcode == OP_BEQ)
    {
        if(regs_[d.rs] == regs_[d.rt])
        {
            pc1=pc1 + (static_cast<uint32_t> (d.imm) << 2);
        }
    }
    else
    {
        throw std::invalid_argument("invalid opcode: " + toHex(d.opcode));
    }
    pc_ = pc1;
}

//memIndex : Byte 주소를 이용해 몇 번째 워드인지 변환해주는 함수, Byte 주소인 addr 과 메모리 크기인 memWords를 이용.
// -> Byte 주소와 memory index는 다르다! memory는 32bit짜리가 1씩 증가하며 vector에 저장되어 있기 때문.
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
