// W4: 분기 trace 생성기
//   패턴별로 "PC(16진수) taken(0/1)" 형식의 trace 파일을 만든다. 예: 0x400120 1
//   첫 줄은 '#' 주석(어떤 인자로 만들었는지)이다. bpred의 로더는 '#' 줄과 빈 줄을 건너뛴다.
//
//   사용법: ./gen_trace <pattern> <length> <out file> [N]
//     pattern : loop | alternate | always_taken | always_not_taken | correlated | mixed
//     length  : 출력할 분기 레코드(줄) 수
//     N       : loop, mixed의 안쪽 루프 taken 횟수 (기본 4). 다른 패턴에서는 무시
//
//   난수가 필요한 패턴(correlated, mixed)은 시드를 고정한 std::mt19937을 쓴다.
//   분포 객체(std::bernoulli_distribution 등)는 표준 라이브러리 구현마다 결과가 다를 수 있으므로
//   mt19937의 출력(표준이 값을 정해 둠)을 직접 % 100 해서 쓴다 → 어느 환경에서 만들어도 같은 trace.
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>

// ─────────────────────────────────────────────────────────────
// 패턴별 PC. 패턴마다 다른 값을 써서 trace를 이어 붙여도 겹치지 않게 한다.
// ─────────────────────────────────────────────────────────────
constexpr uint64_t PC_LOOP        = 0x400120;
constexpr uint64_t PC_ALTERNATE   = 0x400200;
constexpr uint64_t PC_ALWAYS_T    = 0x400300;
constexpr uint64_t PC_ALWAYS_NT   = 0x400340;
constexpr uint64_t PC_CORR_A      = 0x400400;   // 무작위 50%
constexpr uint64_t PC_CORR_B      = 0x400404;   // 바로 앞 A와 같은 결과

// mixed: 루프 본문 안에 분기 5개가 있는 프로그램을 흉내 낸다.
constexpr uint64_t PC_MIX_INNER   = 0x400500;   // 안쪽 루프: N번 taken 후 1번 not-taken
constexpr uint64_t PC_MIX_ALT     = 0x400520;   // 바깥 반복마다 T/N 번갈아
constexpr uint64_t PC_MIX_BIASED  = 0x400540;   // 90% taken (무작위)
constexpr uint64_t PC_MIX_ALWAYS  = 0x400560;   // 항상 taken (바깥 루프의 back edge)
constexpr uint64_t PC_MIX_ALIAS   = 0x401500;   // 항상 not-taken. 하위 PC 비트가 PC_MIX_INNER와 같다
                                                //   → (pc >> 2) & mask가 index bits 10 이하에서 같은 칸으로 간다

constexpr uint32_t RNG_SEED       = 42;

// ─────────────────────────────────────────────────────────────
// 출력 도우미: 한 줄 쓰고, 지금까지 쓴 줄 수를 센다.
//   length에 도달하면 full()이 true → 각 패턴은 full()을 보고 멈춘다.
// ─────────────────────────────────────────────────────────────
class TraceWriter {
public:
    TraceWriter(std::ostream& os, uint64_t length) : os_(os), length_(length) {}

    bool full() const { return written_ >= length_; }

    void emit(uint64_t pc, bool taken) {
        if (full())
        {
            return;
        }
        os_ << "0x" << std::hex << pc << std::dec << " " << (taken ? 1 : 0) << "\n";
        ++written_;
    }

private:
    std::ostream& os_;
    uint64_t      length_;
    uint64_t      written_ = 0;
};

// ─────────────────────────────────────────────────────────────
// 패턴
// ─────────────────────────────────────────────────────────────

// 같은 PC: T × N, NT × 1 반복
static void genLoop(TraceWriter& w, uint64_t n) {
    for (uint64_t i = 0; !w.full(); ++i)
    {
        w.emit(PC_LOOP, (i % (n + 1)) != n);
    }
}

// T, N, T, N, ...
static void genAlternate(TraceWriter& w) {
    for (uint64_t i = 0; !w.full(); ++i)
    {
        w.emit(PC_ALTERNATE, i % 2 == 0);
    }
}

static void genConstant(TraceWriter& w, uint64_t pc, bool taken) {
    while (!w.full())
    {
        w.emit(pc, taken);
    }
}

// if (x) {...}   ← 분기 A: x는 무작위 (어떤 예측기로도 50% 근처)
// if (x) {...}   ← 분기 B: A와 같은 조건 → A의 결과를 알면 B는 확정된다
// 자기 PC의 과거만 보는 예측기(1-bit, 2-bit)는 B도 무작위로 보이지만,
// 전역 히스토리(GHR)에는 직전 A의 결과가 들어 있으므로 gshare는 B를 맞힐 수 있다.
static void genCorrelated(TraceWriter& w) {
    std::mt19937 rng(RNG_SEED);
    while (!w.full())
    {
        bool a = (rng() % 2) == 1;
        w.emit(PC_CORR_A, a);
        w.emit(PC_CORR_B, a);
    }
}

// 바깥 루프 한 번 = 안쪽 루프(N+1개) + alternate + biased + alias + back edge
static void genMixed(TraceWriter& w, uint64_t n) {
    std::mt19937 rng(RNG_SEED);
    for (uint64_t outer = 0; !w.full(); ++outer)
    {
        for (uint64_t i = 0; i <= n; ++i)
        {
            w.emit(PC_MIX_INNER, i != n);
        }
        w.emit(PC_MIX_ALT, outer % 2 == 0);
        w.emit(PC_MIX_BIASED, (rng() % 100) < 90);
        w.emit(PC_MIX_ALIAS, false);
        w.emit(PC_MIX_ALWAYS, true);
    }
}

// ─────────────────────────────────────────────────────────────
// 명령줄 인자
// ─────────────────────────────────────────────────────────────
static void printUsage(const char* prog) {
    std::cerr << "usage: " << prog << " <pattern> <length> <out file> [N]\n"
              << "  pattern: loop | alternate | always_taken | always_not_taken | correlated | mixed\n"
              << "  N      : inner loop taken count for loop/mixed (default 4)\n";
}

// 10진수 양의 정수만 받는다. "12abc", "-3", "" 은 오류.
static uint64_t parsePositive(const std::string& s, const std::string& what) {
    size_t used = 0;
    unsigned long long v = 0;
    try
    {
        if (!s.empty() && s[0] == '-')
        {
            throw std::invalid_argument("negative");
        }
        v = std::stoull(s, &used, 10);
    }
    catch (const std::logic_error&)
    {
        throw std::invalid_argument(what + " must be a positive integer: '" + s + "'");
    }
    if (used != s.size() || v == 0)
    {
        throw std::invalid_argument(what + " must be a positive integer: '" + s + "'");
    }
    return v;
}

int main(int argc, char* argv[]) {
    if (argc < 4 || argc > 5)
    {
        printUsage(argv[0]);
        return 1;
    }

    std::string pattern = argv[1];
    std::string outPath = argv[3];
    uint64_t length = 0;
    uint64_t n = 4;
    try
    {
        length = parsePositive(argv[2], "length");
        if (argc == 5)
        {
            n = parsePositive(argv[4], "N");
        }
    }
    catch (const std::invalid_argument& e)
    {
        std::cerr << "error: " << e.what() << "\n";
        printUsage(argv[0]);
        return 1;
    }

    if (pattern != "loop" && pattern != "alternate" && pattern != "always_taken" &&
        pattern != "always_not_taken" && pattern != "correlated" && pattern != "mixed")
    {
        std::cerr << "error: unknown pattern '" << pattern << "'\n";
        printUsage(argv[0]);
        return 1;
    }

    std::ofstream out(outPath);
    if (!out.is_open())
    {
        std::cerr << "error: cannot open output file: " << outPath << "\n";
        return 1;
    }

    out << "# gen_trace pattern=" << pattern << " length=" << length;
    if (pattern == "loop" || pattern == "mixed")
    {
        out << " N=" << n;
    }
    out << "\n";

    TraceWriter w(out, length);
    if (pattern == "loop")                  genLoop(w, n);
    else if (pattern == "alternate")        genAlternate(w);
    else if (pattern == "always_taken")     genConstant(w, PC_ALWAYS_T, true);
    else if (pattern == "always_not_taken") genConstant(w, PC_ALWAYS_NT, false);
    else if (pattern == "correlated")       genCorrelated(w);
    else                                    genMixed(w, n);

    if (!out)
    {
        std::cerr << "error: failed while writing " << outPath << "\n";
        return 1;
    }
    return 0;
}
