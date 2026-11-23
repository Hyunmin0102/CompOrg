// W6: 캐시 시뮬레이터
//   메모리 접근 trace("R 0x1a2b" / "W 0x1a2b")를 캐시 하나에 흘려 보내고 통계를 출력한다.
//
//   사용법: ./sim --trace=FILE [--size=BYTES] [--assoc=N] [--block=BYTES] [--sweep]
//     --size=BYTES  : 캐시 전체 크기 (기본 32768)
//     --assoc=N     : 연관도. 1이면 direct-mapped (기본 1)
//     --block=BYTES : 블록 크기 (기본 64)
//     --trace=FILE  : trace 파일 (필수)
//     --sweep       : --assoc를 무시하고 연관도 1, 2, 4, 8 결과를 CSV로 출력
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "cache.h"
#include "trace.h"

// ─────────────────────────────────────────────────────────────
// 시뮬레이션과 통계 출력
// ─────────────────────────────────────────────────────────────
// trace의 접근을 순서대로 캐시에 넣는다.
static void run_trace(Cache& cache, const std::vector<Access>& trace) {
    for (const Access& a : trace)
    {
        cache.access(a.addr, a.isWrite);
    }
}

static void print_stats(std::ostream& os, const Stats& s) {
    std::ios oldState(nullptr);
    oldState.copyfmt(os);

    os << "accesses   " << std::setw(10) << s.accesses << "\n"
       << "  reads    " << std::setw(10) << s.reads << "\n"
       << "  writes   " << std::setw(10) << s.writes << "\n"
       << "hits       " << std::setw(10) << s.hits << "\n"
       << "misses     " << std::setw(10) << s.misses << "\n"
       << "writebacks " << std::setw(10) << s.writebacks << "\n"
       << std::fixed << std::setprecision(4)
       << "miss rate  " << std::setw(10) << s.missRate() << "\n";

    os.copyfmt(oldState);
}


// ─────────────────────────────────────────────────────────────
// 명령줄 인자
// ─────────────────────────────────────────────────────────────
struct Options {
    uint64_t    sizeBytes  = 32768;
    uint64_t    assoc      = 1;
    uint64_t    blockBytes = 64;
    std::string tracePath;            // 비어 있으면 usage를 출력하고 끝낸다
    bool        sweep      = false;
};

// 옵션 값 문자열 → uint64_t. 전체가 10진수 숫자일 때만 받는다.
//   std::stoull은 앞의 공백과 '-'를 받아들인다("-1" → 2^64 - 1로 감싸짐). 그래서 첫 글자가 숫자인지 먼저 본다.
//   "12abc"는 예외 없이 12가 되므로 pos(읽은 글자 수)가 문자열 길이와 같은지 확인한다.
//   stoull의 예외: 숫자가 아니면 invalid_argument, 범위를 넘으면 out_of_range (둘 다 logic_error의 자식)
static uint64_t parse_uint(const std::string& value, const std::string& name) {
    const std::string err = name + " needs a non-negative integer, got '" + value + "'";
    if (value.empty() || value[0] < '0' || value[0] > '9')
    {
        throw std::invalid_argument(err);
    }

    size_t pos = 0;
    unsigned long long v = 0;
    try
    {
        v = std::stoull(value, &pos, 10);
    }
    catch (const std::logic_error&)
    {
        throw std::invalid_argument(err);
    }
    if (pos != value.size())
    {
        throw std::invalid_argument(err);
    }
    return static_cast<uint64_t>(v);
}

// --name=value 형식만 받는다. 2의 거듭제곱 검사는 Cache 생성자가 한다.
static Options parse_args(int argc, char* argv[]) {
    Options opt;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        const size_t eq = arg.find('=');

        // '='가 없으면 find는 std::string::npos를 돌려준다 → 값이 없는 플래그
        if (eq == std::string::npos)
        {
            if (arg == "--sweep")
            {
                opt.sweep = true;
                continue;
            }
            throw std::invalid_argument("unknown option '" + arg + "'");
        }

        const std::string name  = arg.substr(0, eq);
        const std::string value = arg.substr(eq + 1);
        if (name == "--size")
        {
            opt.sizeBytes = parse_uint(value, name);
        }
        else if (name == "--assoc")
        {
            opt.assoc = parse_uint(value, name);
        }
        else if (name == "--block")
        {
            opt.blockBytes = parse_uint(value, name);
        }
        else if (name == "--trace")
        {
            if (value.empty())
            {
                throw std::invalid_argument("--trace needs a file path");
            }
            opt.tracePath = value;
        }
        else
        {
            throw std::invalid_argument("unknown option '" + arg + "'");
        }
    }
    return opt;
}

static void print_usage(std::ostream& os, const char* prog) {
    os << "usage: " << prog << " --trace=FILE [--size=BYTES] [--assoc=N] [--block=BYTES] [--sweep]\n";
}


// ─────────────────────────────────────────────────────────────
// --sweep: 연관도 1/2/4/8 비교 CSV
// ─────────────────────────────────────────────────────────────
// 연관도마다 Cache를 새로 만들어 같은 trace를 돌린다 (이전 연관도의 상태가 남지 않게).
// 생성자가 예외를 던지는 연관도는 그 줄만 건너뛰고 이유를 stderr에 쓴다.
//   → stdout(CSV)에는 헤더와 유효한 줄만 남아 그대로 파일로 저장해 쓸 수 있다.
static void print_sweep_csv(std::ostream& os, const Options& opt, const std::vector<Access>& trace) {
    static constexpr uint64_t SWEEP_ASSOCS[] = {1, 2, 4, 8};

    std::ios oldState(nullptr);
    oldState.copyfmt(os);

    os << "size,block,assoc,accesses,hits,misses,writebacks,miss_rate\n";
    os << std::fixed << std::setprecision(4);
    for (const uint64_t assoc : SWEEP_ASSOCS)
    {
        try
        {
            Cache cache(opt.sizeBytes, assoc, opt.blockBytes);
            run_trace(cache, trace);

            const Stats& s = cache.stats();
            os << opt.sizeBytes << "," << opt.blockBytes << "," << assoc << ","
               << s.accesses << "," << s.hits << "," << s.misses << ","
               << s.writebacks << "," << s.missRate() << "\n";
        }
        catch (const std::invalid_argument& e)
        {
            std::cerr << "skip assoc " << assoc << ": " << e.what() << "\n";
        }
    }

    os.copyfmt(oldState);
}


// ─────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    Options opt;
    try
    {
        opt = parse_args(argc, argv);
    }
    catch (const std::exception& e)
    {
        std::cerr << "error: " << e.what() << "\n";
        print_usage(std::cerr, argv[0]);
        return 1;
    }
    if (opt.tracePath.empty())
    {
        print_usage(std::cerr, argv[0]);
        return 1;
    }

    std::vector<Access> trace;
    try
    {
        trace = load_trace(opt.tracePath);
    }
    catch (const std::exception& e)
    {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }

    if (opt.sweep)
    {
        print_sweep_csv(std::cout, opt, trace);
        return 0;
    }

    try
    {
        Cache cache(opt.sizeBytes, opt.assoc, opt.blockBytes);
        run_trace(cache, trace);

        std::cout << "=== cache: size " << opt.sizeBytes << " B, assoc " << opt.assoc
                  << ", block " << opt.blockBytes << " B ===\n"
                  << "trace: " << opt.tracePath << "\n";
        print_stats(std::cout, cache.stats());
    }
    catch (const std::invalid_argument& e)
    {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
