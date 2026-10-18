#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "cpu.h"
#include "decoder.h"

// ─────────────────────────────────────────────────────────────
// 프로그램 로더
//   파일 형식: 한 줄에 16진수 명령어 하나, '#' 이후는 주석, 빈 줄 허용
// ─────────────────────────────────────────────────────────────
std::vector<uint32_t> loadProgramFile(const std::string& path) {
    // TODO: std::ifstream으로 path를 연다. 열기 실패하면 예외를 던진다.
    // TODO: std::getline으로 한 줄씩 읽는다.
    //       - '#' 뒤를 잘라낸다 (std::string::find, substr)
    //       - 공백만 남은 줄은 건너뛴다
    //       - std::stoul(s, nullptr, 16)로 변환한다. 잘못된 입력이면 어떤 예외가 나는가?
    // TODO: 몇 번째 줄에서 오류가 났는지 메시지에 넣으면 디버깅이 쉽다.
    (void)path;
    return {};
}


// ─────────────────────────────────────────────────────────────
// W1 디코더 회귀 테스트 (W1 run_tests()에서 복사)
// ─────────────────────────────────────────────────────────────
static void test_decoder() {
    // 부호 확장 테스트: sign_extend16()은 decoder.cpp 안에 숨겨져 있으므로
    // decode()가 채운 imm 필드로 검사한다. 0x2000xxxx = addi $zero, $zero, 0xxxxx
    assert(decode(0x20000005).imm == 5);
    assert(decode(0x2000FFFF).imm == -1);
    assert(decode(0x20008000).imm == -32768);
    assert(decode(0x20007FFF).imm == 32767);

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


// ─────────────────────────────────────────────────────────────
// 테스트 — 완료 기준: sum1to10 결과 55 (W3 이후 회귀 테스트)
// ─────────────────────────────────────────────────────────────
static void run_tests() {
    test_decoder();

    // TODO: sum1to10 테스트. programs/sum1to10.txt를 읽어 Cpu에서 실행하고, 결과가 55인지 assert한다.
    //       (결과를 어느 레지스터 / 메모리 주소에 둘지 먼저 정한다)
    // TODO: 개별 명령어 테스트. 작은 프로그램(vector)을 직접 만들어 확인한다.
    //       - add / addi (음수 즉시값 포함)
    //       - lw / sw 음수 offset: 예) sw $t0, -4($t1)
    //       - beq 앞으로 가는 분기 / 뒤로 가는 분기 (음수 offset)
    // TODO: 예외 테스트. 정렬 안 된 주소, 메모리 범위 밖, 알 수 없는 opcode에서 throw가 나는지
    //       try/catch로 확인한다.
    // TODO: $zero 불변식 테스트. addi $zero, $zero, 5 실행 후 reg(0) == 0
}


int main(int argc, char* argv[]) {
    run_tests();
    std::cout << "all tests passed\n";

    // TODO: argv[1]에서 프로그램 파일 경로를 받는다 (없으면 사용법 출력 후 종료).
    // TODO: Cpu를 만들고 loadProgramFile() → loadProgram() → run(최대 명령어 수).
    // TODO: try/catch로 예외를 잡아 메시지와 PC(cpu.pc())를 16진수로 출력하고 0이 아닌 값을 반환한다.
    //       Cpu 객체를 try 블록 안에서 만들면 catch에서 cpu.pc()를 쓸 수 있는가? (변수 범위, scope)
    // TODO: 실행 후 dumpRegs()와 결과 메모리 값을 출력한다.
    (void)argc;
    (void)argv;
    return 0;
}
