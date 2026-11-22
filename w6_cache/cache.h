// W6: 캐시 시뮬레이터 (set-associative, LRU, write-back + write-allocate)
//   direct-mapped(v1)는 assoc = 1인 경우다. 별도 클래스를 만들지 않는다.
//   W7에서 L1/L2 계층으로 확장한다: writebackToLower가 하위 레벨 호출로 바뀌는 자리다.
//
//   주소 분해 (offsetBits = log2(blockBytes), indexBits = log2(numSets))
//     | tag | index(set) | offset |
//            ^ indexBits   ^ offsetBits
#pragma once

#include <cstdint>
#include <vector>

// 캐시 라인 하나(= 블록 하나의 메타데이터). 데이터 자체는 저장하지 않는다.
struct Line {
    bool     valid = false, dirty = false;
    uint64_t tag = 0, lastUse = 0;   // lastUse: LRU용 타임스탬프 (마지막으로 쓰인 시각)
};

// 캐시 통계. access() 한 번마다 accesses와 (reads 또는 writes)와 (hits 또는 misses)가 하나씩 는다.
struct Stats {
    uint64_t accesses   = 0;
    uint64_t hits       = 0;
    uint64_t misses     = 0;
    uint64_t reads      = 0;
    uint64_t writes     = 0;
    uint64_t writebacks = 0;   // dirty 라인을 내보낸(evict) 횟수

    // misses / accesses. 접근이 하나도 없으면 0.0
    double missRate() const;
};

class Cache {
public:
    // 셋 다 2의 거듭제곱이 아니거나, sizeBytes가 assoc * blockBytes로 나눠지지 않으면 std::invalid_argument
    Cache(uint64_t sizeBytes, uint64_t assoc, uint64_t blockBytes);

    // 주소 addr에 읽기(isWrite = false) 또는 쓰기(isWrite = true). hit이면 true.
    //   miss면 블록을 가져와 채운다(write miss도 채운다 = write-allocate).
    bool access(uint64_t addr, bool isWrite);

    const Stats& stats() const;

private:
    // 주소 분해
    uint64_t offsetOf(uint64_t addr) const;
    uint64_t indexOf(uint64_t addr) const;
    uint64_t tagOf(uint64_t addr) const;

    // set 안에서 tag가 같은 valid 라인의 way. 없으면 NOT_FOUND
    int findWay(uint64_t set, uint64_t tag) const;
    // miss일 때 채울 way (invalid 라인이 있으면 그것, 없으면 LRU 라인)
    int chooseVictim(uint64_t set) const;
    // dirty 라인을 하위 레벨로 내보낸다. blockAddr = 그 블록의 시작 주소 (offset = 0)
    void writebackToLower(uint64_t blockAddr);

    static constexpr int NOT_FOUND = -1;

    // 멤버는 선언 순서대로 초기화된다. 생성자 초기화 리스트도 이 순서로 쓴다.
    uint64_t sizeBytes_  = 0;
    uint64_t assoc_      = 0;
    uint64_t blockBytes_ = 0;
    uint64_t numSets_    = 0;
    unsigned offsetBits_ = 0;
    unsigned indexBits_  = 0;

    std::vector<Line> lines_;      // 크기 numSets_ * assoc_. 라인 (set, way)는 lines_[set * assoc_ + way]
    uint64_t          useClock_ = 0;   // LRU 타임스탬프 카운터. 접근할 때마다 1씩 는다
    Stats             stats_;

    // tests.cpp가 private 헬퍼(offsetOf 등)와 계산된 값(numSets_ 등)을 직접 확인하기 위한 통로.
    // 외부 인터페이스(access, stats)에는 영향이 없다.
    friend struct CacheTester;
};
