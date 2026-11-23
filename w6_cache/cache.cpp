#include "cache.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include "util.h"

// ─────────────────────────────────────────────────────────────
// Stats
// ─────────────────────────────────────────────────────────────
double Stats::missRate() const {
    if (accesses == 0)
    {
        return 0.0;
    }
    return static_cast<double>(misses) / static_cast<double>(accesses);
}

static unsigned log2_pow2(uint64_t n) {
    unsigned logVal = 0;
    while (n >>= 1) { // 비트를 오른쪽으로 1칸씩 밀면서 카운트
        ++logVal;
    }
    return logVal;
}

// ─────────────────────────────────────────────────────────────
// 생성자
// sizeBytes : 캐시 크기(ex. 32KB), assoc : 연관 수 (ex. 4-way), blockBytes : 블록 크기(ex. 8B)
// numSets : 세트수 (캐시 크기 / way수), offsetBits : 블록을 표현하는데 얼마나 쓰냐, indexBits : 하나의 way 안에 몇 개의 블록이 들어가느냐 or 몇번째 세트에 들어가있냐
// ─────────────────────────────────────────────────────────────
Cache::Cache(uint64_t sizeBytes, uint64_t assoc, uint64_t blockBytes) {
    sizeBytes_ = is_pow2(sizeBytes) ? sizeBytes
        : throw std::invalid_argument("cache size must be a power of 2, got " + std::to_string(sizeBytes));
    assoc_ = is_pow2(assoc) ? assoc
        : throw std::invalid_argument("associativity must be a power of 2, got " + std::to_string(assoc));
    blockBytes_ = is_pow2(blockBytes) ? blockBytes
        : throw std::invalid_argument("block size must be a power of 2, got " + std::to_string(blockBytes));
    numSets_ = (sizeBytes % (assoc * blockBytes) == 0) ? (sizeBytes / (assoc * blockBytes))
        : throw std::invalid_argument("cache size " + std::to_string(sizeBytes)
                                      + " is not a multiple of assoc * block = " + std::to_string(assoc)
                                      + " * " + std::to_string(blockBytes)
                                      + " = " + std::to_string(assoc * blockBytes));
    offsetBits_ = log2_pow2(blockBytes);
    indexBits_ = log2_pow2(numSets_);
    lines_.assign(numSets_ * assoc, Line{});
}


// ─────────────────────────────────────────────────────────────
// 주소 분해
// ─────────────────────────────────────────────────────────────
uint64_t Cache::offsetOf(uint64_t addr) const {
    uint64_t mask = (uint64_t{1} << offsetBits_) - 1;
    return addr & mask;
}

uint64_t Cache::indexOf(uint64_t addr) const {
    uint64_t mask = (uint64_t{1} << indexBits_) - 1;
    return (addr >> offsetBits_) & mask;
}

uint64_t Cache::tagOf(uint64_t addr) const {
    return addr >> (indexBits_ + offsetBits_);
}


// ─────────────────────────────────────────────────────────────
// 라인 찾기 / 교체 대상 고르기
// ─────────────────────────────────────────────────────────────
int Cache::findWay(uint64_t set, uint64_t tag) const {
    for (uint64_t way = 0; way < assoc_; ++way)
    {
        const Line& line = lines_[set * assoc_ + way];
        if (line.valid && line.tag == tag)
        {
            return static_cast<int>(way);
        }
    }
    return NOT_FOUND;
}

int Cache::chooseVictim(uint64_t set) const {
    // 비교 순서: (1) invalid 라인이 먼저 (2) valid가 같으면 lastUse가 작은(오래된) 라인이 먼저
    const auto first = lines_.begin() + set * assoc_;
    const auto last  = first + assoc_;
    const auto it = std::min_element(first, last, [](const Line& a, const Line& b) {
        if (a.valid != b.valid)
        {
            return !a.valid;
        }
        return a.lastUse < b.lastUse;
    });
    return static_cast<int>(it - first);
}

void Cache::writebackToLower([[maybe_unused]] uint64_t blockAddr) {
    // W6: 하위 레벨이 없으므로 개수만 센다.
    // W7: 여기서 하위 캐시(L2)의 access(blockAddr, true)를 부른다.
    ++stats_.writebacks;
}


// ─────────────────────────────────────────────────────────────
// access: 외부 인터페이스
// ─────────────────────────────────────────────────────────────
bool Cache::access(uint64_t addr, bool isWrite) {
    ++stats_.accesses;
    ++useClock_;
    bool isHit = false;
    if (isWrite)
    {
        ++stats_.writes;
    }
    else
    {
        ++stats_.reads;
    }

    const uint64_t set = indexOf(addr);
    const uint64_t tag = tagOf(addr);
    const int way = findWay(set, tag);

    if (way == NOT_FOUND)
    {
        // miss: victim을 내보내고(dirty면 writeback) 새 블록으로 채운다
        ++stats_.misses;
        const int victimWay = chooseVictim(set);
        Line& victim = lines_[set * assoc_ + victimWay];

        // 덮어쓰기 전에 검사: victim.tag가 아직 쫓겨나는 블록의 tag다
        if (victim.valid && victim.dirty)
        {
            const uint64_t victimAddress = (victim.tag << (indexBits_ + offsetBits_)) | (set << offsetBits_);
            writebackToLower(victimAddress);
        }

        victim.valid   = true;
        victim.tag     = tag;
        victim.lastUse = useClock_;
        victim.dirty   = isWrite;   // write-allocate: 쓰기 miss면 dirty, 읽기 miss면 clean
    }
    else
    {
        // hit: LRU 시각 갱신, 쓰기면 dirty (읽기 hit은 dirty를 건드리지 않는다)
        ++stats_.hits;
        Line& line = lines_[set * assoc_ + way];
        line.lastUse = useClock_;
        isHit = true;

        if (isWrite)
        {
            line.dirty = true;
        }
    }

    return isHit;
}

const Stats& Cache::stats() const {
    return stats_;
}
