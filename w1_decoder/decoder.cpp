#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>

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

constexpr uint8_t OP_R     = 0;
constexpr uint8_t OP_J     = 2;
constexpr uint8_t OP_JAL   = 3;
constexpr uint8_t OP_ADDI  = 8;
constexpr uint8_t OP_LW    = 35;
constexpr uint8_t OP_SW    = 43;
constexpr uint8_t OP_BEQ   = 4;
constexpr uint8_t OP_BNE   = 5;

constexpr uint8_t FUNCT_ADD = 32;
constexpr uint8_t FUNCT_SUB = 34;
constexpr uint8_t FUNCT_AND = 36;
constexpr uint8_t FUNCT_OR  = 37;
constexpr uint8_t FUNCT_SLT = 42;
constexpr uint8_t FUNCT_SLL = 0;
constexpr uint8_t FUNCT_SRL = 2;
constexpr uint8_t FUNCT_JR = 8;

// ─────────────────────────────────────────────────────────────
// 2. 명령어 형식
// ─────────────────────────────────────────────────────────────
enum class Format { R, I, J, Unknown };


// ─────────────────────────────────────────────────────────────
// 3. 디코딩 결과
//    `= 0` 같은 기본 멤버 초기화는 W3에서 본격적으로 다룬다. 지금은
//    "초기화 안 된 쓰레기 값이 남지 않게 하는 장치" 정도로만 이해하면 된다.
// ─────────────────────────────────────────────────────────────
struct DecodedInst {
    uint32_t raw    = 0;               // 원래 32비트 명령어
    Format   format = Format::Unknown;
    uint8_t  opcode = 0;
    uint8_t  rs = 0, rt = 0, rd = 0;
    uint8_t  shamt = 0, funct = 0;
    int32_t  imm    = 0;               // I형: 부호 확장된 16비트 즉시값
    uint32_t target = 0;               // J형: 26비트 주소 필드 (아직 << 2 하지 않은 값)
};


// ─────────────────────────────────────────────────────────────
// 4. 비트 필드 추출 헬퍼
//    inst를 shift만큼 오른쪽으로 민 뒤, mask로 하위 비트만 남긴다.
// ─────────────────────────────────────────────────────────────
uint32_t field(uint32_t inst, unsigned shift, uint32_t mask) {
    return (inst >> shift) & mask;
}


// ─────────────────────────────────────────────────────────────
// 5. 16비트 → 32비트 부호 확장 (sign extension)
//    예: 0x0005 → 5,  0xFFFF → -1,  0x8000 → -32768
// ─────────────────────────────────────────────────────────────
int32_t sign_extend16(uint32_t imm16) {
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
Format format_of(uint8_t opcode) {
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


// ─────────────────────────────────────────────────────────────
// 10. 테스트 — 완료 기준: 명령어 10개가 강의/교재 표와 일치
// ─────────────────────────────────────────────────────────────
void run_tests() {
    // 부호 확장 단위 테스트
    assert(sign_extend16(0x0005) == 5);
    assert(sign_extend16(0xFFFF) == -1);
    assert(sign_extend16(0x8000) == -32768);
    assert(sign_extend16(0x7FFF) == 32767);

    // [1] add $s0, $s1, $s2   
    {
        DecodedInst d = decode(0x02328020);
        assert(d.format == Format::R);
        assert(d.opcode == 0);
        assert(d.rs == 17 && d.rt == 18 && d.rd == 16);
        assert(d.shamt == 0 && d.funct == 32);
        assert(mnemonic(d) == "add");
    }

    // [2] addi $s0, $s1, 5  
    {
        DecodedInst d = decode(0x22300005);
        assert(d.format == Format::I);
        assert(d.opcode == 8);
        assert(d.rs == 17 && d.rt == 16);
        assert(d.imm == 5);
        assert(mnemonic(d) == "addi");
    }

    // [3] lw $s3, 8($s0)      — L7 슬라이드 67 (필드 값: op=35 rs=16 rt=19 imm=8)
    {
        DecodedInst d = decode(0x8E130008);
        assert(d.format == Format::I);
        assert(d.opcode == 35);
        assert(d.rs == 16);
        assert(d.rt == 19);
        assert(d.imm == 8);
        assert(mnemonic(d) == "lw");
    }

    // [4] sub $t0, $t1, $t2  →  $t0 = $t1 - $t2
    {
        DecodedInst d = decode(0x012A4022);
        assert(d.format == Format::R);
        assert(d.opcode == OP_R);
        assert(d.rs == 9);   // $t1 (소스1)
        assert(d.rt == 10);  // $t2 (소스2)
        assert(d.rd == 8);   // $t0 (목적지)
        assert(d.shamt == 0 && d.funct == FUNCT_SUB);
        assert(mnemonic(d) == "sub");
    }

    // [5] sll $t0, $t1, 4  →  $t0 = $t1 << 4
    {
        DecodedInst d = decode(0x00094100);
        assert(d.format == Format::R);
        assert(d.opcode == OP_R);
        assert(d.rs == 0);   // 사용 안 함
        assert(d.rt == 9);   // $t1 (시프트할 값)
        assert(d.rd == 8);   // $t0 (목적지)
        assert(d.shamt == 4);
        assert(d.funct == FUNCT_SLL);
        assert(mnemonic(d) == "sll");
    }

    // [6] sw $t0, 12($sp)  →  Memory[$sp + 12] = $t0
    {
        DecodedInst d = decode(0xAFA8000C);
        assert(d.format == Format::I);
        assert(d.opcode == OP_SW);
        assert(d.rs == 29);  // $sp (base)
        assert(d.rt == 8);   // $t0 (저장할 값)
        assert(d.imm == 12);
        assert(mnemonic(d) == "sw");
    }

    // [7] addi $t0, $t0, -1
    {
        DecodedInst d = decode(0x2108FFFF);
        assert(d.format == Format::I);
        assert(d.opcode == OP_ADDI);
        assert(d.rs == 8); // $t0
        assert(d.rt == 8); // $t0
        assert(d.imm == -1);
        assert(mnemonic(d) == "addi");
    }

    // [8] beq $t0, $zero, -3
    {
        DecodedInst d = decode(0x1100FFFD);
        assert(d.format == Format::I);
        assert(d.opcode == OP_BEQ);
        assert(d.rs == 8); // $t0
        assert(d.rt == 0); // $zero
        assert(d.imm == -3);
        assert(mnemonic(d) == "beq");
    }

    // [9] j 0x00400000  →  target 필드 = 0x00400000 >> 2 = 0x0100000
    {
        DecodedInst d = decode(0x08100000);
        assert(d.format == Format::J);
        assert(d.opcode == OP_J);
        assert(d.target == (0x00400000 >> 2));
        assert(mnemonic(d) == "j");
    }

    // [10] jr $ra
    {
        DecodedInst d = decode(0x03E00008);
        assert(d.format == Format::R);
        assert(d.opcode == OP_R);
        assert(d.rs == 31); // $ra
        assert(d.rt == 0 && d.rd == 0 && d.shamt == 0);
        assert(d.funct == FUNCT_JR);
        assert(mnemonic(d) == "jr");
    }
}


int main() {
    run_tests();
    std::cout << "all tests passed\n";

    // 16진수 명령어를 한 줄씩 읽어서 디코딩. std::stoul(s, nullptr, 16)은 "0x" 접두사도 허용한다.
    // 잘못된 입력("zzz")이면 예외가 던져진다 — try/catch는 W2에서 다루니 지금은 신경 쓰지 않아도 된다.
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        uint32_t inst = static_cast<uint32_t>(std::stoul(line, nullptr, 16));
        print(decode(inst));
    }
    return 0;
}