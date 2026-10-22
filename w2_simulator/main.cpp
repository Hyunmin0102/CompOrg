#include <cassert>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "cpu.h"
#include "decoder.h"

// ─────────────────────────────────────────────────────────────
// 프로그램 로더
//   파일 형식: 한 줄에 16진수 명령어 하나, '#' 이후는 주석, 빈 줄 허용
// ─────────────────────────────────────────────────────────────
std::vector<uint32_t> loadProgramFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("cannot open program file: " + path);
    }

    std::vector<uint32_t> program;
    std::string line;
    size_t lineNo = 0;
    while (std::getline(file, line))
    {
        lineNo++;

        // '#' 뒤는 주석. '#'이 없으면 find가 npos를 돌려주고, substr(0, npos)는 줄 전체다.
        std::string code = line.substr(0, line.find('#'));

        // 앞뒤 공백 제거. getline은 '\n'만 지우므로 Windows 줄 끝의 '\r'도 공백으로 취급한다.
        // 공백이 아닌 글자가 없으면(빈 줄, 주석만 있는 줄, 들여쓴 주석) 건너뛴다.
        const char* blanks = " \t\r";
        size_t first = code.find_first_not_of(blanks);
        if (first == std::string::npos)
        {
            continue;
        }
        size_t last = code.find_last_not_of(blanks);
        code = code.substr(first, last - first + 1);

        // 오류 메시지 앞에 붙일 위치 정보 (예: "programs/sum1to10.txt:5: ")
        std::string where = path + ":" + std::to_string(lineNo) + ": ";

        unsigned long value = 0;
        size_t used = 0;   // stoul이 숫자로 읽은 글자 수
        try
        {
            value = std::stoul(code, &used, 16);
        }
        catch (const std::logic_error&)   // invalid_argument("zzz"), out_of_range(64비트 초과) 둘 다
        {
            throw std::invalid_argument(where + "invalid hex '" + code + "'");
        }

        // stoul은 읽을 수 있는 데까지만 읽고 나머지는 말없이 무시한다 ("2008zz0A" → 0x2008).
        // 다 읽지 못했으면 오타로 보고 예외를 던진다.
        if (used != code.size())
        {
            throw std::invalid_argument(where + "invalid hex '" + code + "'");
        }

        // stoul의 반환 타입 unsigned long은 64비트 → 32비트 명령어 범위를 직접 검사한 뒤 변환한다.
        if (value > 0xFFFFFFFF)
        {
            throw std::out_of_range(where + "value exceeds 32 bits '" + code + "'");
        }
        program.push_back(static_cast<uint32_t>(value));
    }
    return program;
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
// 예외 테스트 패턴
//   assert는 "조건이 참인가"만 확인한다. "이 호출이 예외를 던져야 한다"를 확인하려면
//   bool 변수 + try/catch를 쓴다.
//     1) threw = false로 시작
//     2) try 안에서 예외가 나야 하는 호출
//     3) 기대한 타입의 예외를 catch하면 threw = true
//     4) assert(threw) — 예외가 안 났으면 여기서 실패한다 (이 줄이 없으면 조용히 통과해 버린다)
//   catch에 구체적인 타입을 쓰면 "맞는 종류의 예외인지"까지 확인된다.
//   다른 타입이 던져지면 잡히지 않고 프로그램이 멈추므로 그것도 실패로 드러난다.
//   예외 객체를 쓰지 않으므로 catch에 변수 이름을 붙이지 않는다 (붙이면 unused 경고).
// ─────────────────────────────────────────────────────────────


// ─────────────────────────────────────────────────────────────
// 1단계: 생성자, reg(), pc()
// ─────────────────────────────────────────────────────────────
static void test_init() {
    // [1] 새 Cpu의 레지스터는 모두 0, PC는 0
    {
        Cpu cpu(4, 4);
        for (int i = 0; i < 32; ++i) {
            assert(cpu.reg(i) == 0);
        }
        assert(cpu.pc() == 0);
    }

    // [2] reg(32): 범위 밖 → std::out_of_range (.at()이 던짐)
    {
        Cpu cpu(4, 4);
        bool threw = false;
        try {
            cpu.reg(32);
        } catch (const std::out_of_range&) {
            threw = true;
        }
        assert(threw);
    }

    // [3] reg(-1): int -1이 size_t로 바뀌면 아주 큰 수 → 역시 std::out_of_range
    {
        Cpu cpu(4, 4);
        bool threw = false;
        try {
            cpu.reg(-1);
        } catch (const std::out_of_range&) {
            threw = true;
        }
        assert(threw);
    }
}


// ─────────────────────────────────────────────────────────────
// 2단계: loadWord(), storeWord()  (memIndex는 private이라 이 둘을 통해 간접 확인)
//   데이터 메모리 4워드 = 16바이트 → 유효한 주소는 0, 4, 8, 12
// ─────────────────────────────────────────────────────────────
static void test_memory() {
    // [1] 새 Cpu의 데이터 메모리는 0 (생성자 초기화 확인)
    {
        Cpu cpu(4, 4);
        assert(cpu.loadWord(0) == 0);
        assert(cpu.loadWord(12) == 0);
    }

    // [2] 쓴 값을 그대로 읽어 온다
    {
        Cpu cpu(4, 4);
        cpu.storeWord(8, 123);
        assert(cpu.loadWord(8) == 123);
    }

    // [3] 마지막 칸(주소 12)에 큰 값 — 경계값 + 32비트 전체 사용
    {
        Cpu cpu(4, 4);
        cpu.storeWord(12, 0xFFFFFFFF);
        assert(cpu.loadWord(12) == 0xFFFFFFFF);
    }

    // [4] 한 칸에 쓴 뒤 옆 칸은 그대로 0 — 엉뚱한 칸(인덱스 계산 실수)에 쓰지 않았는지
    {
        Cpu cpu(4, 4);
        cpu.storeWord(4, 7);
        assert(cpu.loadWord(0) == 0);
        assert(cpu.loadWord(4) == 7);
        assert(cpu.loadWord(8) == 0);
    }

    // [5] 주소 16: 정렬은 맞지만 범위 밖 (경계값) → std::out_of_range
    {
        Cpu cpu(4, 4);
        bool threw = false;
        try {
            cpu.loadWord(16);
        } catch (const std::out_of_range&) {
            threw = true;
        }
        assert(threw);
    }

    // [6] 주소 0xFFFFFFFC: lw에서 0 + (-4)를 하면 나오는 주소 → std::out_of_range
    {
        Cpu cpu(4, 4);
        bool threw = false;
        try {
            cpu.loadWord(0xFFFFFFFC);
        } catch (const std::out_of_range&) {
            threw = true;
        }
        assert(threw);
    }

    // [7] 주소 6: 4의 배수가 아님 → std::invalid_argument (읽기)
    {
        Cpu cpu(4, 4);
        bool threw = false;
        try {
            cpu.loadWord(6);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        assert(threw);
    }

    // [8] 주소 2: 4의 배수가 아님 → std::invalid_argument (쓰기)
    {
        Cpu cpu(4, 4);
        bool threw = false;
        try {
            cpu.storeWord(2, 1);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        assert(threw);
    }
}


// ─────────────────────────────────────────────────────────────
// 3단계: loadProgram(), done()  (fetch/decodeInst는 private → 4단계 step() 테스트에서 확인)
//   명령어 값은 아직 실행하지 않으므로 아무 값이나 된다. test_decoder의 값을 재사용.
// ─────────────────────────────────────────────────────────────
static void test_load() {
    // [1] 3워드 적재 직후: PC는 첫 명령어(주소 0), 아직 끝나지 않음
    {
        Cpu cpu(4, 4);
        cpu.loadProgram({0x22300005, 0x02328020, 0x8E130008});
        assert(cpu.pc() == 0);
        assert(!cpu.done());
    }

    // [2] 빈 프로그램: 실행할 게 없으므로 바로 끝
    {
        Cpu cpu(4, 4);
        cpu.loadProgram({});
        assert(cpu.done());
    }

    // [3] imem 크기(4워드)와 같은 프로그램: 딱 들어간다 (경계값, 예외 없음)
    {
        Cpu cpu(4, 4);
        cpu.loadProgram({0x22300005, 0x02328020, 0x8E130008, 0xAFA8000C});
        assert(cpu.pc() == 0);
        assert(!cpu.done());
    }

    // [4] imem보다 1워드 큰 프로그램 → 예외
    //     loadProgram()에서 던지는 타입(std::out_of_range)에 맞춰 catch한다.
    //     예외 타입을 바꾸면 여기도 같이 바꿔야 한다.
    {
        Cpu cpu(4, 4);
        bool threw = false;
        try {
            cpu.loadProgram({0x22300005, 0x02328020, 0x8E130008, 0xAFA8000C, 0x2108FFFF});
        } catch (const std::out_of_range&) {
            threw = true;
        }
        assert(threw);
    }
}


// ─────────────────────────────────────────────────────────────
// W2 완료 기준: sum1to10 (W3 이후 회귀 테스트)
//   programs/sum1to10.txt를 실제로 읽어 끝까지 실행하고 결과를 고정한다.
//   프로그램 구조: $t0 = 10 → 0 으로 세면서 $t1에 더함 → sw $t1, -4($s0) / lw $t6, -4($s0)  ($s0 = 8)
//   경로는 실행 위치 기준 상대 경로이므로 w2_simulator/ 에서 ./sim 을 실행해야 한다.
// ─────────────────────────────────────────────────────────────
static void test_sum1to10() {
    Cpu cpu(256, 256);   // trace는 켜지 않는다 (테스트 출력이 지저분해지지 않게)
    cpu.loadProgram(loadProgramFile("programs/sum1to10.txt"));
    cpu.run(1000);       // 정상이면 46개. 무한 루프면 runtime_error로 테스트가 멈춘다

    assert(cpu.done());                // 프로그램 끝까지 실행됨
    assert(cpu.reg(9) == 55);          // $t1: 1 + 2 + ... + 10
    assert(cpu.reg(8) == 0);           // $t0: 카운터가 0까지 내려와 반복문을 빠져나옴
    assert(cpu.reg(16) == 8);          // $s0: sw/lw의 base
    assert(cpu.loadWord(4) == 55);     // sw $t1, -4($s0) → 주소 8 + (-4) = 4 (음수 offset)
    assert(cpu.reg(14) == 55);         // lw $t6, -4($s0) → 같은 주소에서 다시 읽음
    assert(cpu.reg(0) == 0);           // $zero 불변식
    assert(cpu.instCount() == 46);     // 2 + 반복 10번 × 4 + 마지막 beq 1 + 3
}


// ─────────────────────────────────────────────────────────────
// 테스트 — 완료 기준: sum1to10 결과 55 (W3 이후 회귀 테스트)
// ─────────────────────────────────────────────────────────────
static void run_tests() {
    test_decoder();
    test_init();
    test_memory();
    test_load();
    test_sum1to10();

    // TODO: 개별 명령어 테스트. 작은 프로그램(vector)을 직접 만들어 확인한다.
    //       - add / addi (음수 즉시값 포함)
    //       - lw / sw 음수 offset: 예) sw $t0, -4($t1)
    //       - beq 앞으로 가는 분기 / 뒤로 가는 분기 (음수 offset)
    // TODO: 예외 테스트. 정렬 안 된 주소, 메모리 범위 밖, 알 수 없는 opcode에서 throw가 나는지
    //       try/catch로 확인한다.
    // TODO: $zero 불변식 테스트. addi $zero, $zero, 5 실행 후 reg(0) == 0
}


// ─────────────────────────────────────────────────────────────
// main: 테스트를 돌린 뒤, 인자로 받은 프로그램 파일을 실행하고 결과를 출력한다.
//   사용법: ./sim <프로그램 파일>        (make run = ./sim programs/sum1to10.txt)
// ─────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    run_tests();
    std::cout << "all tests passed\n";

    // argc는 프로그램 이름까지 센다. "./sim"만 실행하면 argc == 1이고 argv[0] == "./sim"이다.
    // 파일 경로가 없으면 사용법을 출력하고 0이 아닌 값을 반환한다 (0 = 정상 종료, 셸에서 echo $?로 확인).
    if (argc < 2)
    {
        std::cerr << "usage: " << argv[0] << " <program file>\n";
        return 1;
    }

    // 숫자를 코드에 바로 쓰지 않고 이름을 붙인다. 바꿀 때도 여기 한 곳만 고치면 된다.
    constexpr size_t   IMEM_WORDS  = 256;    // 명령어 메모리 256워드 = 1KB
    constexpr size_t   DMEM_WORDS  = 256;    // 데이터 메모리 256워드 = 1KB
    constexpr size_t   MAX_INSTS   = 10000;  // 무한 루프 방지 한도 (sum1to10은 수십 개면 끝난다)
    constexpr uint32_t RESULT_ADDR = 4;      // sum1to10이 sw로 결과를 저장하는 주소

    // Cpu는 try 블록 "밖"에서 만든다.
    // try { } 안에서 선언한 변수는 그 블록이 끝나면 사라지므로(scope), catch에서 cpu.pc()를 쓸 수 없다.
    // 생성자는 vector 두 개를 만들 뿐이라 여기서 예외가 날 일은 사실상 없다.
    Cpu cpu(IMEM_WORDS, DMEM_WORDS);
    cpu.setTrace(true);   // 실행되는 명령어를 [PC] + 디코드 결과로 출력

    try
    {
        // 파일 → vector<uint32_t> → 명령어 메모리. 형식 오류면 loadProgramFile이 줄 번호와 함께 던진다.
        cpu.loadProgram(loadProgramFile(argv[1]));
        // done()이 될 때까지 step() 반복. MAX_INSTS를 넘으면 runtime_error.
        cpu.run(MAX_INSTS);
    }
    catch (const std::exception& e)
    {
        // 지금까지 던진 invalid_argument, out_of_range, runtime_error는 모두 std::exception의 자식이라
        // 부모 타입 하나로 전부 잡힌다. what()은 throw할 때 넣은 메시지를 돌려준다.
        //
        // PC는 예외를 일으킨 명령어의 주소다. execute()가 PC를 맨 끝에서만 갱신하기 때문에
        // 예외가 나면 PC가 그대로 남는다. (파일 읽기 단계에서 실패했다면 PC는 초기값 0)
        // std::hex / std::setfill은 끈적(sticky)하므로 출력 뒤 std::dec / ' '로 되돌린다.
        std::cerr << "error: " << e.what() << "\n"
                  << "  PC = 0x" << std::hex << std::setw(8) << std::setfill('0') << cpu.pc()
                  << std::dec << std::setfill(' ')
                  << ", executed " << cpu.instCount() << " instructions\n";
        return 1;
    }

    // 정상 종료: 레지스터 덤프 + 실행한 명령어 수 + 결과 메모리 값
    std::cout << "\n=== registers ===\n";
    cpu.dumpRegs();

    std::cout << "\n=== result ===\n"
              << "instructions executed: " << cpu.instCount() << "\n"
              << "mem[" << RESULT_ADDR << "] = " << cpu.loadWord(RESULT_ADDR) << "\n";
    return 0;
}
