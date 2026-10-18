// W1 decoder.cpp에서 옮겨 온 디코더 구현. 함수 본문은 W1 그대로다.
// field / sign_extend16 / format_of는 decode 내부에서만 쓰므로 static으로 숨긴다
// (static 함수는 이 .cpp 파일 밖에서 보이지 않는다 = internal linkage).
#include "decoder.h"

#include <iomanip>
#include <iostream>

// ─────────────────────────────────────────────────────────────
// 1. 필드 위치 상수  (L7 슬라이드 61·106의 비트 번호 그림 참고)
// ─────────────────────────────────────────────────────────────
constexpr unsigned OPCODE_SHIFT = 26;     // 31:26
constexpr uint32_t OPCODE_MASK  = 0x3F;   // 6비트
constexpr unsigned RS_SHIFT     = 21;     // 25:21
constexpr uint32_t RS_MASK      = 0x1F;   // 5비트
constexpr unsigned RT_SHIFT     = 16;     // 20:16
constexpr uint32_t RT_MASK      = 0x1F;   // 5비트
constexpr unsigned RD_SHIFT     = 11;     // 15:11
constexpr uint32_t RD_MASK      = 0x1F;   // 5비트
constexpr unsigned SHAMT_SHIFT  = 6;      // 10:6
constexpr uint32_t SHAMT_MASK   = 0x1F;   // 5비트
constexpr unsigned FUNCT_SHIFT  = 0;      // 5:0
constexpr uint32_t FUNCT_MASK   = 0x3F;   // 6비트

constexpr unsigned IMM_SHIFT    = 0;      // 15:0
constexpr uint32_t IMM_MASK     = 0xFFFF; // 16비트

constexpr unsigned TARGET_SHIFT = 0;      // 25:0
constexpr uint32_t TARGET_MASK  = 0x3FFFFFF; // 26비트


// ─────────────────────────────────────────────────────────────
// 4. 비트 필드 추출 헬퍼
//    inst를 shift만큼 오른쪽으로 민 뒤, mask로 하위 비트만 남긴다.
// ─────────────────────────────────────────────────────────────
static uint32_t field(uint32_t inst, unsigned shift, uint32_t mask) {
    return (inst >> shift) & mask;
}


// ─────────────────────────────────────────────────────────────
// 5. 16비트 → 32비트 부호 확장 (sign extension)
//    예: 0x0005 → 5,  0xFFFF → -1,  0x8000 → -32768
// ─────────────────────────────────────────────────────────────
static int32_t sign_extend16(uint32_t imm16) {
    // //방법 1 직접 변환하는 방법
    // if((imm16 & 0x8000) == 0x8000)
    // {
    //     int32_t imm = static_cast<int32_t> (imm16 | 0xFFFF0000);
    //     return imm;
    // }
    // else
    // {
    //     return static_cast<int32_t> (imm16);
    // }
    //방법 2 : 형변환을 이용하는 방법
    int16_t imm16_signed = static_cast<int16_t> (imm16);
    return static_cast<int32_t> (imm16_signed);

}


// ─────────────────────────────────────────────────────────────
// 6. opcode → 형식
// ─────────────────────────────────────────────────────────────
static Format format_of(uint8_t opcode) {
    switch (opcode)
    {
    case OP_R:
        return Format::R;
    case OP_J:
    case OP_JAL:
        return Format::J;
    case OP_LW:
    case OP_SW:
    case OP_BEQ:
    case OP_ADDI:
    case OP_BNE:
        return Format::I;
    default:
        return Format::Unknown;
    }
}


// ─────────────────────────────────────────────────────────────
// 7. 디코드 본체
// ─────────────────────────────────────────────────────────────
DecodedInst decode(uint32_t inst) {
    DecodedInst d;
    // 실제 하드웨어처럼 모든 필드를 추출, 어떤 필드를 쓸지는 format으로 판단
    d.raw    = inst;
    d.opcode = static_cast<uint8_t>(field(inst, OPCODE_SHIFT, OPCODE_MASK));
    d.rs     = static_cast<uint8_t>(field(inst, RS_SHIFT, RS_MASK));
    d.rt     = static_cast<uint8_t>(field(inst, RT_SHIFT, RT_MASK));
    d.rd     = static_cast<uint8_t>(field(inst, RD_SHIFT, RD_MASK));
    d.shamt  = static_cast<uint8_t>(field(inst, SHAMT_SHIFT, SHAMT_MASK));
    d.funct  = static_cast<uint8_t>(field(inst, FUNCT_SHIFT, FUNCT_MASK));
    d.imm    = sign_extend16(field(inst, IMM_SHIFT, IMM_MASK));
    d.target = field(inst, TARGET_SHIFT, TARGET_MASK);
    d.format = format_of(d.opcode);

    return d;
}


// ─────────────────────────────────────────────────────────────
// 8. 명령어 이름 (디스어셈블러의 첫 단계)
// ─────────────────────────────────────────────────────────────
std::string mnemonic(const DecodedInst& d) {
    switch(d.format)
    {
    case Format::R:
        switch(d.funct)
        {
            case FUNCT_SLL: return "sll";
            case FUNCT_SRL: return "srl";
            case 3: return "sra";
            case 4: return "sllv";
            case 6: return "srlv";
            case 7: return "srav";
            case FUNCT_JR: return "jr";
            case 9: return "jalr";
            case 12: return "syscall";
            case 13: return "break";
            case 16: return "mfhi";
            case 17: return "mthi";
            case 18: return "mflo";
            case 19: return "mtlo";
            case 24: return "mult";
            case 25: return "multu";
            case 26: return "div";
            case 27: return "divu";
            case FUNCT_ADD: return "add";
            case 33: return "addu";
            case FUNCT_SUB: return "sub";
            case 35: return "subu";
            case FUNCT_AND: return "and";
            case FUNCT_OR: return "or";
            case 38: return "xor";
            case 39: return "nor";
            case FUNCT_SLT: return "slt";
            case 43: return "sltu";
            default: return "unknown";
        }
    case Format::I:
        switch(d.opcode)
        {
        case OP_LW: return "lw";
        case OP_SW: return "sw";
        case OP_BEQ: return "beq";
        case OP_BNE: return "bne";
        case OP_ADDI: return "addi";
        default: return "unknown";
        }
    case Format::J:
        switch (d.opcode)
        {
        case OP_J: return "j";
        case OP_JAL: return "jal";
        default: return "unknown";
        }
    case Format::Unknown: return "unknown";
    }
    return "unknown";
}


// ─────────────────────────────────────────────────────────────
// 9. 출력
// ─────────────────────────────────────────────────────────────
void print(const DecodedInst& d) {
    std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << d.raw
              << std::dec << std::setfill(' ')      // std::hex는 "끈적"하다. 되돌려 놓기.
              << "  " << mnemonic(d) << '\n';
    switch(d.format)
    {
    case Format::R:
        std::cout << "R  opcode=" << static_cast<int>(d.opcode) << " rs=" << static_cast<int>(d.rs)
        << " rt=" << static_cast<int>(d.rt) << " rd=" << static_cast<int>(d.rd)
        << " shamt=" << static_cast<int>(d.shamt) << " funct=" << static_cast<int>(d.funct) << "\n";
        break;
    case Format::I:
        std::cout << "I  opcode=" << static_cast<int>(d.opcode) << " rs=" << static_cast<int>(d.rs)
        << " rt=" << static_cast<int>(d.rt) << " imm=" << d.imm << "\n";
        break;
    case Format::J:
        std::cout << "J  opcode=" << static_cast<int>(d.opcode) << " target=" << "0x"
        << std::hex << std::setw(7) << std::setfill('0') << d.target << "\n" << std::dec
        << std::setfill(' ');
        break;
    case Format::Unknown:
        std::cout << "?  opcode=" << static_cast<int>(d.opcode) << "\n";
        break;
    }
    // uint8_t는 unsigned char이라 static_cast 이용해서 int로 바꾸어 출력해야함.
}
