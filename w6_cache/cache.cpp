#include "cache.h"

#include <algorithm>
#include <stdexcept>

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


// ─────────────────────────────────────────────────────────────
// 생성자
// ─────────────────────────────────────────────────────────────
Cache::Cache([[maybe_unused]] uint64_t sizeBytes, [[maybe_unused]] uint64_t assoc,
             [[maybe_unused]] uint64_t blockBytes) {
    // TODO(2): 파라미터 검사와 예외, numSets/offsetBits/indexBits 계산 (tests: ./test ctor)
    //   - 검사 (하나라도 걸리면 std::invalid_argument, 메시지에 어떤 값이 왜 틀렸는지)
    //       sizeBytes, assoc, blockBytes가 모두 2의 거듭제곱인가 → is_pow2 (util.h)
    //       sizeBytes가 assoc * blockBytes로 나눠지는가 (작으면 나눠지지 않는다)
    //   - 계산
    //       numSets_    = sizeBytes / (assoc * blockBytes)
    //       offsetBits_ = log2(blockBytes), indexBits_ = log2(numSets_)
    //       2의 거듭제곱의 log2 = "1을 몇 번 왼쪽으로 밀어야 그 값이 되는가" → 반복문 또는 오른쪽 시프트로
    //       (<cmath>의 std::log2는 double을 돌려주므로 쓰지 않는다)
    //   - lines_를 numSets_ * assoc_ 칸으로 만든다. Line의 기본값(valid = false)이 그대로 초기값이다
    //   - 멤버 초기화 리스트를 쓸 거라면 cache.h의 선언 순서를 지킨다(-Wall이 -Wreorder로 알려 준다).
    //     단, 검사는 본문에서 하므로 "검사 전에 계산해도 안전한 값"만 초기화 리스트에 둔다.
    //   - 구현이 끝나면 [[maybe_unused]]와 이 TODO 주석을 지운다.
}


// ─────────────────────────────────────────────────────────────
// 주소 분해
// ─────────────────────────────────────────────────────────────
uint64_t Cache::offsetOf([[maybe_unused]] uint64_t addr) const {
    // TODO(3): 블록 안 위치 = 주소의 하위 offsetBits_ 비트 (tests: ./test addr)
    //   - mask = (1을 offsetBits_만큼 민 값) - 1. 1은 64비트 정수로 시작한다 (W4 함정: 1 << n은 int 시프트)
    return 0;
}

uint64_t Cache::indexOf([[maybe_unused]] uint64_t addr) const {
    // TODO(3): set 번호 = offset을 오른쪽으로 밀어 낸 뒤 하위 indexBits_ 비트
    //   - 경계 조건: indexBits_ == 0 (fully associative, set이 하나)일 때 항상 0이 나오는가
    return 0;
}

uint64_t Cache::tagOf([[maybe_unused]] uint64_t addr) const {
    // TODO(3): tag = offset과 index를 모두 밀어 낸 나머지 상위 비트
    return 0;
}


// ─────────────────────────────────────────────────────────────
// 라인 찾기 / 교체 대상 고르기
// ─────────────────────────────────────────────────────────────
int Cache::findWay([[maybe_unused]] uint64_t set, [[maybe_unused]] uint64_t tag) const {
    // TODO(4): set의 way 0..assoc_-1 중 valid이고 tag가 같은 라인을 찾는다 (tests: ./test v1)
    //   - 라인 (set, way)는 lines_[set * assoc_ + way]
    //   - 반복 변수 타입과 assoc_(uint64_t)의 비교에서 -Wsign-compare 경고가 나면 어느 쪽 타입을 맞출지 생각한다
    //   - 없으면 NOT_FOUND
    return NOT_FOUND;
}

int Cache::chooseVictim([[maybe_unused]] uint64_t set) const {
    // TODO(5): LRU 교체 대상 (tests: ./test lru)
    //   - set의 라인들은 lines_에서 연속된 assoc_칸이다 → 시작 반복자 begin() + set * assoc_, 끝 = 시작 + assoc_
    //   - std::min_element(시작, 끝, 람다)로 lastUse가 가장 작은 라인을 찾는다
    //     람다: [](const Line& a, const Line& b) { ... } — "a가 b보다 앞(더 작다)인가"를 돌려준다
    //   - 돌려준 반복자 - 시작 = way 번호 (반복자끼리 빼면 거리가 나온다)
    //   - invalid 라인이 있으면 그것을 먼저 골라야 한다.
    //     따로 검사해도 되고, 람다의 비교 규칙에 valid를 넣어도 된다. lastUse 초기값(0)과
    //     useClock_의 시작값을 어떻게 정하면 되는지도 생각해 볼 것
    //   - v1(assoc = 1)에서는 way가 하나뿐이므로 항상 0이다. TODO(4) 단계에서는 임시로 0을 돌려줘도 된다
    return 0;
}

void Cache::writebackToLower([[maybe_unused]] uint64_t blockAddr) {
    // W6: 하위 레벨이 없으므로 개수만 센다.
    // W7: 여기서 하위 캐시(L2)의 access(blockAddr, true)를 부른다.
    ++stats_.writebacks;
}


// ─────────────────────────────────────────────────────────────
// access: 외부 인터페이스
// ─────────────────────────────────────────────────────────────
bool Cache::access([[maybe_unused]] uint64_t addr, [[maybe_unused]] bool isWrite) {
    // TODO(4): hit/miss 판정 (v1: assoc = 1) (tests: ./test v1)
    //   - 통계: accesses, reads/writes를 센다
    //   - addr → set(indexOf), tag(tagOf) → findWay
    //   - hit:  hits를 센다, true
    //   - miss: misses를 센다, 채울 way(chooseVictim)에 valid = true, tag를 쓴다, false
    //
    // TODO(5): LRU 타임스탬프 (tests: ./test lru)
    //   - 접근할 때마다 useClock_을 올리고, hit한 라인과 새로 채운 라인의 lastUse에 기록한다
    //
    // TODO(6): write-back + dirty bit (tests: ./test writeback)
    //   - 쓰기(hit이든 miss든)는 그 라인의 dirty = true. 하위 레벨에는 아직 쓰지 않는다(write-back)
    //   - miss에서 victim이 valid && dirty이면, 덮어쓰기 전에 writebackToLower(victim 블록의 시작 주소)
    //     victim의 블록 주소는 victim의 tag와 지금 set 번호로 다시 만든다 (offset = 0)
    //     → | tag | set | 0...0 | : 각 부분을 몇 비트씩 밀어야 하는가?
    //   - 새로 채운 라인의 dirty는 이번 접근이 쓰기인지에 따라 정한다
    //   - 구현이 끝나면 [[maybe_unused]]와 TODO 주석, 임시 return을 지운다.
    return false;
}

const Stats& Cache::stats() const {
    return stats_;
}
