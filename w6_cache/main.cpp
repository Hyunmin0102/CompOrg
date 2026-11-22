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

static Options parse_args([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    Options opt;
    // TODO(7): 명령줄 옵션 파싱 (확인: README "TODO(7) 완료 확인")
    //   - argv[1] .. argv[argc - 1]을 std::string으로 바꿔 하나씩 본다
    //   - "--size=4096"을 이름 "--size"와 값 "4096"으로 나눈다
    //       std::string::find('=')로 위치를 찾고, substr로 앞/뒤를 자른다
    //       '='가 없으면 find가 무엇을 돌려주는가? (--sweep은 '=' 없이 온다)
    //   - 값은 std::stoul로 숫자로 바꾼다
    //       std::stoul이 던지는 예외 두 가지는? "12abc"는 예외 없이 12가 된다 → 두 번째 인자(pos)로 확인
    //       "-1"을 넣으면 어떻게 되는가? (W5 main.cpp의 parseUnsigned 참고)
    //   - 모르는 옵션, 값이 없는 옵션("--size=")은 std::invalid_argument
    //   - 값이 2의 거듭제곱인지는 여기서 검사하지 않는다. Cache 생성자가 검사한다
    //   - 구현이 끝나면 [[maybe_unused]]와 이 TODO 주석을 지운다.
    return opt;
}

static void print_usage(std::ostream& os, const char* prog) {
    os << "usage: " << prog << " --trace=FILE [--size=BYTES] [--assoc=N] [--block=BYTES] [--sweep]\n";
}


// ─────────────────────────────────────────────────────────────
// --sweep: 연관도 1/2/4/8 비교 CSV
// ─────────────────────────────────────────────────────────────
static void print_sweep_csv([[maybe_unused]] std::ostream& os, [[maybe_unused]] const Options& opt,
                            [[maybe_unused]] const std::vector<Access>& trace) {
    // TODO(8): 연관도 1, 2, 4, 8 결과를 CSV로 출력 (확인: README "TODO(8) 완료 확인")
    //   - 출력 형식 (첫 줄은 헤더, 쉼표 뒤 공백 없음, miss_rate는 소수점 아래 4자리)
    //       size,block,assoc,accesses,hits,misses,writebacks,miss_rate
    //       32768,64,1,<accesses>,<hits>,<misses>,<writebacks>,<miss_rate>   ← 연관도마다 한 줄
    //   - 연관도마다 Cache를 새로 만든다 (이전 연관도의 상태가 남으면 안 된다)
    //   - 연관도 목록을 배열이나 vector로 두고 범위 기반 for로 돈다
    //   - std::fixed / std::setprecision은 sticky다 → print_stats처럼 서식을 저장·복원한다
    //   - Cache 생성자가 예외를 던지는 연관도(예: size가 작아서 안 나눠짐)는 어떻게 할지 정한다
    //     (그 줄만 건너뛰기 / 전체 중단). 정한 방식을 README에 적는다
    //   - 구현이 끝나면 [[maybe_unused]]와 이 TODO 주석을 지운다.
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
