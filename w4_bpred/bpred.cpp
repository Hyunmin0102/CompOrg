// W4: 분기 예측기 시뮬레이터 (1-bit, 2-bit 포화 카운터, gshare)
//   trace(한 줄에 "PC taken")를 읽어서 세 예측기의 정확도를 비교한다.
//
//   사용법: ./bpred [--index-bits N] [--history-bits H] [--top] <trace file>...
//     --index-bits N   : 예측 테이블 크기 2^N (기본 10)
//     --history-bits H : gshare 전역 히스토리 길이 (기본 10, 0..N)
//     --top            : trace·예측기별로 오예측이 많은 PC Top 10 출력
//
//   이번 주는 세 예측기를 일부러 서로 독립된 클래스로 만든다 (공통 기반 클래스·가상 함수 없음).
//   그래서 클래스 선언과 simulate 함수가 거의 같은 모양으로 세 번 나온다 → W5에서 추상 클래스로 합친다.
//   이 중복이 "어디서, 왜" 생기는지 직접 겪어 보는 것이 이번 주 목적 중 하나다.
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

constexpr int      COUNTER_MAX     = 3;
constexpr int      COUNTER_MIN     = 0;
constexpr int      WEAKLY_TAKEN    = 2;
constexpr int      TAKEN_THRESHOLD = 2;

// ─────────────────────────────────────────────────────────────
// trace 레코드
// ─────────────────────────────────────────────────────────────
struct BranchRecord {
    uint64_t pc    = 0;
    bool     taken = false;
};
 
// trace 한 줄("0x400120 1")을 BranchRecord로 바꾼다.
// 호출하는 쪽(loadTrace)이 '#' 주석 줄과 빈 줄은 이미 걸러 낸다.
BranchRecord parseTraceLine(const std::string& line) {
    std::istringstream iss(line); // line을 읽어와 공백을 기준으로 나눔.

    BranchRecord br;
    iss >> std::hex >> br.pc >> std::dec >> br.taken; //hex 모드의 operator>>는 0x, 0X를 받아줌.
    if (iss.fail())
    {
        throw std::invalid_argument("Can't read two values correctly"); // taken 이 0 or 1이 아니거나 pc가 16진수가 아니면 -> invalid argument
    }
    iss >> std::ws;
    if (!iss.eof())
    {
        throw std::invalid_argument("Useless string left");
    }
    return br;  
}

// trace 파일 전체를 읽는다. '#' 이후는 주석, 빈 줄 허용.
// 파싱 오류는 "파일:줄번호: " 를 앞에 붙여 다시 던진다.
std::vector<BranchRecord> loadTrace(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("cannot open trace file: " + path);
    }

    std::vector<BranchRecord> trace;
    std::string line;
    size_t lineNo = 0;
    while (std::getline(file, line))
    {
        lineNo++;
        std::string code = line.substr(0, line.find('#'));
        if (code.find_first_not_of(" \t\r") == std::string::npos)
        {
            continue;
        }
        try
        {
            trace.push_back(parseTraceLine(code));
        }
        catch (const std::exception& e)
        {
            throw std::invalid_argument(path + ":" + std::to_string(lineNo) + ": " + e.what());
        }
    }
    return trace;
}


// ─────────────────────────────────────────────────────────────
// 예측기 1: 1-bit (PC별로 마지막 결과를 기억)
//   테이블: std::vector<uint8_t>, 크기 2^indexBits, 칸마다 0(not-taken) 또는 1(taken)
//   인덱스: (pc >> 2) & mask   — 명령어가 4바이트 정렬이라 하위 2비트는 항상 0이므로 버린다
// ─────────────────────────────────────────────────────────────
class OneBitPredictor {
public:
    explicit OneBitPredictor(unsigned indexBits);

    bool predict(uint64_t pc) const;        // true = taken으로 예측
    void update(uint64_t pc, bool taken);   // 실제 결과로 테이블 갱신

private:
    size_t index(uint64_t pc) const;

    std::vector<uint8_t> table_; //
    uint64_t             mask_;
};

//루프 : backedge가 많으므로 table_의 taken값은 1로 초기화.
OneBitPredictor::OneBitPredictor(unsigned indexBits)
    : table_(uint64_t{1}<<indexBits, 1), mask_(table_.size()-1) {

}

size_t OneBitPredictor::index(uint64_t pc) const {
    return static_cast<size_t>((pc >> 2) & mask_);
}

bool OneBitPredictor::predict(uint64_t pc) const {
    return table_.at(index(pc)); // 지금은 table_에 0 또는 1만 들어가서 상관없지만, 2-bit에서는 11,10,01,00 등의 상태가 들어가므로 01 의 상태에서도 true가 나올 수 있음.
}

void OneBitPredictor::update(uint64_t pc, bool taken) {
    table_.at(index(pc)) = taken;
}


// ─────────────────────────────────────────────────────────────
// 예측기 2: 2-bit 포화 카운터 (saturating counter)
//   칸마다 0..3. 0,1 = not-taken 예측 / 2,3 = taken 예측
//   (0 strongly NT, 1 weakly NT, 2 weakly T, 3 strongly T)
//   인덱스는 1-bit와 같다.
// ─────────────────────────────────────────────────────────────
class TwoBitPredictor {
public:
    explicit TwoBitPredictor(unsigned indexBits);

    bool predict(uint64_t pc) const;
    void update(uint64_t pc, bool taken);

private:
    size_t index(uint64_t pc) const;

    std::vector<uint8_t> table_;
    uint64_t             mask_;
};

// table 내부 값을 2(weakly taken)으로 정한 이유 : always taken이든 always not taken이든 가장 정확도가 높다. 
// 다른 이유도 있겠지만 일반적으로 weakly taken으로 정하므로 일단 2로 정하고 이유는 구현 이후 생각해보겠음.
TwoBitPredictor::TwoBitPredictor(unsigned indexBits)
    : table_(uint64_t(1)<<indexBits, WEAKLY_TAKEN), mask_(table_.size()-1) {
}

//이 함수는 1-bit index와 동일하다.
size_t TwoBitPredictor::index(uint64_t pc) const {
    return static_cast<size_t>((pc >> 2) & mask_);
}

bool TwoBitPredictor::predict(uint64_t pc) const {
    return table_.at(index(pc)) >= TAKEN_THRESHOLD;
}

void TwoBitPredictor::update(uint64_t pc, bool taken) {
    uint8_t& counter = table_.at(index(pc));
    int count = static_cast<int>(counter);
    if (taken) {
        counter = static_cast<uint8_t>(std::min(count + 1, COUNTER_MAX));
    }
    else {
        counter = static_cast<uint8_t>(std::max(count - 1, COUNTER_MIN));
    }
}


// ─────────────────────────────────────────────────────────────
// 예측기 3: gshare
//   칸은 2-bit 포화 카운터. 전역 히스토리 레지스터(GHR, global history register)에
//   최근 historyBits개 분기의 결과(PC와 상관없이)를 비트로 모은다.
//   인덱스: ((pc >> 2) ^ ghr) & mask
// ─────────────────────────────────────────────────────────────
class GsharePredictor {
public:
    GsharePredictor(unsigned indexBits, unsigned historyBits); //historyBits : 최근 몇 개의 분기를 볼 건지 여부

    bool predict(uint64_t pc) const;
    void update(uint64_t pc, bool taken);   // 카운터 갱신 + GHR 갱신

private:
    size_t index(uint64_t pc) const;

    std::vector<uint8_t> table_;
    uint64_t             mask_;
    uint64_t             ghr_;
    uint64_t             historyMask_;   // GHR을 historyBits 비트로 자르는 mask
};

// table_ 칸 초깃값: 2-bit와 같은 WEAKLY_TAKEN (두 예측기를 같은 조건에서 비교하기 위해).
// ghr_ 초깃값 0: 아직 실행된 분기가 없으므로 "최근 분기가 모두 not-taken"으로 본다.
//   분기마다 한 비트씩 밀려 나가므로 historyBits번 분기 뒤에는 초깃값의 영향이 사라진다.
GsharePredictor::GsharePredictor(unsigned indexBits, unsigned historyBits)
    : table_(uint64_t(1) << indexBits, WEAKLY_TAKEN),
      mask_(table_.size() - 1),
      ghr_(0),
      historyMask_((uint64_t(1) << historyBits) - 1) {}

size_t GsharePredictor::index(uint64_t pc) const {
    return static_cast<size_t>(((pc >> 2) ^ ghr_) & mask_);
}

bool GsharePredictor::predict(uint64_t pc) const {
    return table_.at(index(pc)) >= TAKEN_THRESHOLD;
}

//predict 할 때는 지금까지의 값을 이용해 예측을 하고, predict 이후에는 지금 값을 포함해 update 해야함.
void GsharePredictor::update(uint64_t pc, bool taken) {
    uint8_t& counter = table_.at(index(pc));
    int count = static_cast<int>(counter);
    if (taken) {
        counter = static_cast<uint8_t>(std::min(count + 1, COUNTER_MAX));
    }
    else {
        counter = static_cast<uint8_t>(std::max(count - 1, COUNTER_MIN));
    }
    ghr_ = ((ghr_ << 1) | taken) & historyMask_;
}


// ─────────────────────────────────────────────────────────────
// 시뮬레이션 결과
// ─────────────────────────────────────────────────────────────
struct Result {
    std::string name;                                     // "1-bit", "2-bit", "gshare"
    uint64_t    total   = 0;                              // 예측한 분기 수
    uint64_t    correct = 0;                              // 맞힌 수
    std::unordered_map<uint64_t, uint64_t> mispredByPc;   // PC → 오예측 횟수 (어느 분기가 몇 번 틀렸냐)

    // 정확도(%). 분기가 하나도 없으면 0.0 (출력에서는 n/a로 보인다)
    double accuracy() const {
        if (total == 0)
        {
            return 0.0;
        }
        return 100.0 * static_cast<double>(correct) / static_cast<double>(total);
    }
};

// trace 하나를 예측기 하나로 돌린다. 예측기마다 함수가 따로 있다 (공통 기반 클래스가 없으므로).
//   세 함수의 본문은 예측기 타입과 생성자 인자만 다르고 나머지는 같다 → W5에서 하나로 합칠 자리.
Result simulateOneBit(const std::vector<BranchRecord>& trace, unsigned indexBits) {
    OneBitPredictor pred(indexBits);
    Result r;
    //trace : 테스트할 분기를 전체 다 모아둔 것.
    for(const auto& rec : trace)
    {
        r.total += 1;
        bool is_right = (pred.predict(rec.pc) == rec.taken);
        if (is_right)
        {
            r.correct+=1;
        }
        else
        {
            r.mispredByPc[rec.pc] += 1;
        }
        pred.update(rec.pc, rec.taken);
    }

    r.name = "1-bit";
    return r;
}

Result simulateTwoBit(const std::vector<BranchRecord>& trace, unsigned indexBits) {
    TwoBitPredictor pred(indexBits);
    Result r;
    for (const auto& rec : trace)
    {
        ++r.total;
        bool isRight = (pred.predict(rec.pc) == rec.taken);
        if (isRight)
        {
            ++r.correct;
        }
        else
        {
            ++r.mispredByPc[rec.pc];
        }
        pred.update(rec.pc, rec.taken);
    }

    r.name = "2-bit";
    return r;
}

Result simulateGshare(const std::vector<BranchRecord>& trace, unsigned indexBits, unsigned historyBits) {
    GsharePredictor pred(indexBits, historyBits);
    Result r;       
    for (const auto& rec : trace)
    {
        ++r.total;
        bool isRight = (pred.predict(rec.pc) == rec.taken);
        if (isRight)
        {
            ++r.correct;
        }
        else
        {
            ++r.mispredByPc[rec.pc];
        }
        pred.update(rec.pc, rec.taken);
    }
    r.name = "gshare";
    return r;
}


// ─────────────────────────────────────────────────────────────
// 오예측 Top K
// ─────────────────────────────────────────────────────────────
// 오예측이 많은 순서로 최대 k개의 (PC, 횟수)를 돌려준다.
std::vector<std::pair<uint64_t, uint64_t>> topMispredicted(
        const std::unordered_map<uint64_t, uint64_t>& mispredByPc, size_t k) {

    // std::vector<std::pair<uint64_t, uint64_t>> vec;
    // for (const auto& elem : mispredByPc)
    // {
    //     vec.push_back({elem.first, elem.second});
    // }
    
    // 방법 B: 반복자 범위 생성자. [begin, end) 사이 원소를 전부 복사해서 vector를 만든다.
    //   pair<const uint64_t, uint64_t>(map 원소) → pair<uint64_t, uint64_t>(vector 원소)로 하나씩 변환된다.
    //   {}가 아니라 ()를 쓴다: {}면 "반복자 두 개짜리 initializer_list"로 해석될 수 있다.
    std::vector<std::pair<uint64_t, uint64_t>> vec(mispredByPc.begin(), mispredByPc.end());
    std::sort(vec.begin(), vec.end(), [](const auto& a, const auto& b)  // a,b는 pair
    {
        if(a.second == b.second)
        {
            return a.first < b.first; // second가 같으면 PC로 봐야하는데, PC는 보통 오름차순으로 읽음
        }
        return a.second > b.second;
    });
    vec.resize(std::min(k, vec.size()));
    return vec;
}

// 예측기 하나의 Top K를 출력한다. 예:
//     1-bit  top 10 mispredicted
//       0x400120        10
void printTopMispredicted(std::ostream& os, const Result& r, size_t k) {
    // std::hex 같은 서식은 sticky라서 호출자의 os에 남는다 → 시작할 때 저장하고 끝에서 되돌린다
    std::ios oldState(nullptr);
    oldState.copyfmt(os);

    os << "    " << r.name << "  top " << k << " mispredicted\n";

    const std::vector<std::pair<uint64_t, uint64_t>> top = topMispredicted(r.mispredByPc, k);
    if (top.empty()) {
        os << "      (none)\n";   // 오예측이 하나도 없을 때 (예: always_taken)
    }
    for (const auto& [pc, cnt] : top) {
        // PC는 16진수, 횟수는 10진수. std::dec으로 되돌리지 않으면 cnt도 16진수로 나온다
        os << "      0x" << std::hex << pc << std::dec << std::setw(10) << cnt << "\n";
    }

    os.copyfmt(oldState);
}


// ─────────────────────────────────────────────────────────────
// 결과 표 출력
// ─────────────────────────────────────────────────────────────
struct TraceResult {
    std::string path;
    uint64_t    branches = 0;   // trace의 레코드 수
    Result      oneBit;
    Result      twoBit;
    Result      gshare;
};

// 정확도 칸 하나. total이 0이면(분기가 없거나, 아직 simulate를 구현하지 않았으면) n/a
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
       << std::setw(10) << "branches"
       << std::setw(10) << "1-bit"
       << std::setw(10) << "2-bit"
       << std::setw(10) << "gshare" << "\n";

    os << std::fixed << std::setprecision(2);
    for (const TraceResult& row : rows)
    {
        os << std::left << std::setw(static_cast<int>(nameWidth)) << row.path << std::right
           << std::setw(10) << row.branches;
        printAccuracyCell(os, row.oneBit);
        printAccuracyCell(os, row.twoBit);
        printAccuracyCell(os, row.gshare);
        os << "\n";
    }

    os.copyfmt(oldState);
}


// ─────────────────────────────────────────────────────────────
// 명령줄 인자
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
// main: trace마다 세 예측기를 돌리고 비교 표를 출력한다.
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
        row.oneBit   = simulateOneBit(trace, opt.indexBits);
        row.twoBit   = simulateTwoBit(trace, opt.indexBits);
        row.gshare   = simulateGshare(trace, opt.indexBits, opt.historyBits);
        rows.push_back(row);
    }

    printAccuracyTable(std::cout, rows, opt.indexBits, opt.historyBits);

    if (opt.showTop)
    {
        for (const TraceResult& row : rows)
        {
            std::cout << "\n=== " << row.path << " ===\n";
            printTopMispredicted(std::cout, row.oneBit, TOP_K);
            printTopMispredicted(std::cout, row.twoBit, TOP_K);
            printTopMispredicted(std::cout, row.gshare, TOP_K);
        }
    }
    return 0;
}
