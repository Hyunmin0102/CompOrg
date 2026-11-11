// W5: 분기 예측기 시뮬레이터 (W4 bpred.cpp를 추상 클래스 + 파일 분리로 리팩터링)
//   trace(한 줄에 "PC taken")를 읽어서 등록된 예측기들의 정확도를 비교한다.
//   출력은 W4 bpred와 글자 하나까지 같아야 한다 (compare_w4.sh로 확인).
//
//   사용법: ./bpred [--index-bits N] [--history-bits H] [--top] <trace file>...
//     --index-bits N   : 예측 테이블 크기 2^N (기본 10)
//     --history-bits H : gshare 전역 히스토리 길이 (기본 = N, 0..N)
//     --top            : trace·예측기별로 오예측이 많은 PC Top 10 출력
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "branch_predictor.h"
#include "predictor_factory.h"
#include "trace.h"

// ─────────────────────────────────────────────────────────────
// 시뮬레이션 결과 (W4와 같다)
// ─────────────────────────────────────────────────────────────
struct Result {
    std::string name;                                     // 예측기 이름 (BranchPredictor::name)
    uint64_t    total   = 0;                              // 예측한 분기 수
    uint64_t    correct = 0;                              // 맞힌 수
    std::unordered_map<uint64_t, uint64_t> mispredByPc;   // PC → 오예측 횟수

    // 정확도(%). 분기가 하나도 없으면 0.0 (출력에서는 n/a로 보인다)
    double accuracy() const {
        if (total == 0)
        {
            return 0.0;
        }
        return 100.0 * static_cast<double>(correct) / static_cast<double>(total);
    }
};

// TODO: trace 하나를 예측기 하나로 돌린다. W4의 simulateOneBit / simulateTwoBit / simulateGshare를 하나로 합친다
//   - 예측기를 기반 클래스 참조(reference to base)로 받는다: 동적 바인딩(dynamic binding)
//   - 결과 이름은 예측기에게 묻는다
Result simulate(BranchPredictor& pred, const std::vector<BranchRecord>& trace) {
    (void)pred;
    (void)trace;
    return Result{};   // 임시 반환값
}


// ─────────────────────────────────────────────────────────────
// 오예측 Top K (W4에서 그대로 옮김)
// ─────────────────────────────────────────────────────────────
// 오예측이 많은 순서로 최대 k개의 (PC, 횟수)를 돌려준다.
std::vector<std::pair<uint64_t, uint64_t>> topMispredicted(
        const std::unordered_map<uint64_t, uint64_t>& mispredByPc, size_t k) {
    std::vector<std::pair<uint64_t, uint64_t>> vec(mispredByPc.begin(), mispredByPc.end());
    std::sort(vec.begin(), vec.end(), [](const auto& a, const auto& b)  // a,b는 pair
    {
        if (a.second == b.second)
        {
            return a.first < b.first; // 횟수가 같으면 PC 오름차순
        }
        return a.second > b.second;
    });
    vec.resize(std::min(k, vec.size()));
    return vec;
}

// 예측기 하나의 Top K를 출력한다.
void printTopMispredicted(std::ostream& os, const Result& r, size_t k) {
    // std::hex 같은 서식은 sticky라서 호출자의 os에 남는다 → 시작할 때 저장하고 끝에서 되돌린다
    std::ios oldState(nullptr);
    oldState.copyfmt(os);

    os << "    " << r.name << "  top " << k << " mispredicted\n";

    const std::vector<std::pair<uint64_t, uint64_t>> top = topMispredicted(r.mispredByPc, k);
    if (top.empty()) {
        os << "      (none)\n";
    }
    for (const auto& [pc, cnt] : top) {
        os << "      0x" << std::hex << pc << std::dec << std::setw(10) << cnt << "\n";
    }

    os.copyfmt(oldState);
}


// ─────────────────────────────────────────────────────────────
// 결과 표 출력
//   W4는 예측기마다 필드(oneBit, twoBit, gshare)가 따로 있었다.
//   W5는 예측기 개수를 모르므로 Result를 vector에 담고, 열 이름은 Result::name에서 가져온다.
// ─────────────────────────────────────────────────────────────
struct TraceResult {
    std::string         path;
    uint64_t            branches = 0;   // trace의 레코드 수
    std::vector<Result> results;        // 예측기 순서 = predictor_names() 순서
};

// 정확도 칸 하나. total이 0이면 n/a
static void printAccuracyCell(std::ostream& os, const Result& r) {
    if (r.total == 0)
    {
        os << std::setw(10) << "n/a";
    }
    else
    {
        os << std::setw(9) << r.accuracy() << "%";
    }
}

void printAccuracyTable(std::ostream& os, const std::vector<TraceResult>& rows,
                        unsigned indexBits, unsigned historyBits) {
    std::ios oldState(nullptr);
    oldState.copyfmt(os);

    // trace 이름 칸 너비: 가장 긴 경로에 맞춘다 (최소 5 = "trace")
    size_t nameWidth = 5;
    for (const TraceResult& row : rows)
    {
        nameWidth = std::max(nameWidth, row.path.size());
    }

    os << "=== accuracy (index bits = " << indexBits
       << ", table entries = " << (uint64_t{1} << indexBits)
       << ", gshare history bits = " << historyBits << ") ===\n";
    os << std::left << std::setw(static_cast<int>(nameWidth)) << "trace" << std::right
       << std::setw(10) << "branches";
    if (!rows.empty())
    {
        for (const Result& r : rows.front().results)
        {
            os << std::setw(10) << r.name;
        }
    }
    os << "\n";

    os << std::fixed << std::setprecision(2);
    for (const TraceResult& row : rows)
    {
        os << std::left << std::setw(static_cast<int>(nameWidth)) << row.path << std::right
           << std::setw(10) << row.branches;
        for (const Result& r : row.results)
        {
            printAccuracyCell(os, r);
        }
        os << "\n";
    }

    os.copyfmt(oldState);
}


// ─────────────────────────────────────────────────────────────
// 명령줄 인자 (W4에서 그대로 옮김)
//   ./bpred [--index-bits N] [--history-bits H] [--top] <trace file>...
// ─────────────────────────────────────────────────────────────
constexpr unsigned MAX_INDEX_BITS = 24;   // 2^24칸 = 16MB. 그 이상은 이 과제에서 필요 없다
constexpr size_t   TOP_K          = 10;

struct Options {
    unsigned                 indexBits   = 10;
    unsigned                 historyBits = 10;
    bool                     showTop     = false;
    std::vector<std::string> tracePaths;
};

// 10진수 부호 없는 정수. "12abc", "-1", "" 은 오류
static unsigned parseUnsigned(const std::string& s, const std::string& option) {
    size_t used = 0;
    unsigned long v = 0;
    try
    {
        if (!s.empty() && s[0] == '-')
        {
            throw std::invalid_argument("negative");
        }
        v = std::stoul(s, &used, 10);
    }
    catch (const std::logic_error&)
    {
        throw std::invalid_argument(option + " needs a non-negative integer, got '" + s + "'");
    }
    if (used != s.size() || v > MAX_INDEX_BITS)
    {
        throw std::invalid_argument(option + " needs an integer in 0.." + std::to_string(MAX_INDEX_BITS) +
                                    ", got '" + s + "'");
    }
    return static_cast<unsigned>(v);
}

static Options parseArgs(int argc, char* argv[]) {
    Options opt;
    bool historyGiven = false;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--index-bits" || arg == "--history-bits")
        {
            if (i + 1 >= argc)
            {
                throw std::invalid_argument(arg + " needs a value");
            }
            unsigned v = parseUnsigned(argv[++i], arg);
            if (arg == "--index-bits")
            {
                opt.indexBits = v;
            }
            else
            {
                opt.historyBits = v;
                historyGiven = true;
            }
        }
        else if (arg == "--top")
        {
            opt.showTop = true;
        }
        else if (arg.compare(0, 2, "--") == 0)
        {
            throw std::invalid_argument("unknown option '" + arg + "'");
        }
        else
        {
            opt.tracePaths.push_back(arg);
        }
    }

    // 히스토리를 따로 주지 않으면 인덱스 비트 수에 맞춘다 (gshare의 흔한 설정)
    if (!historyGiven)
    {
        opt.historyBits = opt.indexBits;
    }
    if (opt.indexBits == 0)
    {
        throw std::invalid_argument("--index-bits must be at least 1");
    }
    if (opt.historyBits > opt.indexBits)
    {
        throw std::invalid_argument("--history-bits (" + std::to_string(opt.historyBits) +
                                    ") must not exceed --index-bits (" + std::to_string(opt.indexBits) + ")");
    }
    return opt;
}

static void printUsage(std::ostream& os, const char* prog) {
    os << "usage: " << prog << " [--index-bits N] [--history-bits H] [--top] <trace file>...\n";
}


// ─────────────────────────────────────────────────────────────
// main: trace마다 등록된 예측기를 모두 돌리고 비교 표를 출력한다.
// ─────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    Options opt;
    try
    {
        opt = parseArgs(argc, argv);
    }
    catch (const std::invalid_argument& e)
    {
        std::cerr << "error: " << e.what() << "\n";
        printUsage(std::cerr, argv[0]);
        return 1;
    }
    if (opt.tracePaths.empty())
    {
        printUsage(std::cerr, argv[0]);
        return 1;
    }

    // TODO: opt에서 PredictorConfig를 채운다

    std::vector<TraceResult> rows;
    for (const std::string& path : opt.tracePaths)
    {
        std::vector<BranchRecord> trace;
        try
        {
            trace = loadTrace(path);
        }
        catch (const std::exception& e)
        {
            std::cerr << "error: " << e.what() << "\n";
            return 1;
        }

        TraceResult row;
        row.path     = path;
        row.branches = trace.size();

        // TODO: 예측기 목록을 만들고 다형성(polymorphism)으로 순회해 row.results를 채운다
        //   - 컨테이너: std::vector<std::unique_ptr<BranchPredictor>>
        //   - 담기: make_predictor, std::move (unique_ptr는 복사할 수 없다)
        //   - 확인: std::move가 꼭 필요한 경우와 없어도 되는 경우를 구분해 본다
        //   - 순회: 범위 기반 for, 역참조(*)로 simulate에 넘긴다
        //   - 확인: 예측기 상태가 trace마다 처음부터 시작해야 W4와 결과가 같다

        rows.push_back(row);
    }

    printAccuracyTable(std::cout, rows, opt.indexBits, opt.historyBits);

    if (opt.showTop)
    {
        for (const TraceResult& row : rows)
        {
            std::cout << "\n=== " << row.path << " ===\n";
            for (const Result& r : row.results)
            {
                printTopMispredicted(std::cout, r, TOP_K);
            }
        }
    }
    return 0;
}
