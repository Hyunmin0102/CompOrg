// W1 decoder.cpp에서 디코더 부분을 옮겨 온 헤더.
// 함수는 선언만 둔다. 정의를 헤더에 두면 이 파일을 include하는 .cpp마다 정의가 생겨
// 링크할 때 multiple definition 에러가 난다.
#ifndef DECODER_H
#define DECODER_H

#include <cstdint>
#include <string>

// ─────────────────────────────────────────────────────────────
// opcode / funct 상수 (cpu.cpp의 execute에서도 쓰므로 헤더에 둔다)
// 네임스페이스 범위의 constexpr 변수는 internal linkage라 헤더에 둬도 안전하다.
// ─────────────────────────────────────────────────────────────
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
// 명령어 형식
// ─────────────────────────────────────────────────────────────
enum class Format { R, I, J, Unknown };


// ─────────────────────────────────────────────────────────────
// 디코딩 결과
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


DecodedInst decode(uint32_t inst);          // 32비트 명령어 → 필드 분해
std::string mnemonic(const DecodedInst& d); // 명령어 이름
void        print(const DecodedInst& d);    // 16진수 + 이름 + 필드 출력

#endif // DECODER_H
