// W6: 캐시 시뮬레이터 단계별 테스트 (assert)
//   사용법: ./test <stage>
//     pow2       TODO(1)  is_pow2
//     ctor       TODO(2)  생성자 검사·예외, numSets/offsetBits/indexBits
//     addr       TODO(3)  offsetOf / indexOf / tagOf
//     v1         TODO(4)  direct-mapped hit/miss (충돌 miss)
//     lru        TODO(5)  2-way LRU 교체 순서
//     writeback  TODO(6)  dirty bit, eviction 때 writeback 수
//     v2         = lru + writeback
//     trace      (완성됨) load_trace
//     all        위 전부 (pow2 → ctor → addr → v1 → v2 → trace)
//
//   assert가 실패하면 그 자리에서 멈추고 파일:줄번호를 출력한다. 그 줄의 주석에 계산 과정이 있다.
//   tests/ 아래 파일을 상대 경로로 읽으므로 w6_cache 폴더에서 실행한다.
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "cache.h"
#include "trace.h"
#include "util.h"

// cache.h에서 friend로 선언했다. private 멤버를 테스트에서만 읽는다.
struct CacheTester {
    static uint64_t numSets(const Cache& c)    { return c.numSets_; }
    static unsigned offsetBits(const Cache& c) { return c.offsetBits_; }
    static unsigned indexBits(const Cache& c)  { return c.indexBits_; }
    static size_t   lineCount(const Cache& c)  { return c.lines_.size(); }

    static uint64_t offsetOf(const Cache& c, uint64_t addr) { return c.offsetOf(addr); }
    static uint64_t indexOf(const Cache& c, uint64_t addr)  { return c.indexOf(addr); }
    static uint64_t tagOf(const Cache& c, uint64_t addr)    { return c.tagOf(addr); }
};

// f()가 std::invalid_argument를 던지면 true. 다른 예외나 예외 없음은 false
template <typename F>
bool throws_invalid_argument(F f) {
    try
    {
        f();
    }
    catch (const std::invalid_argument&)
    {
        return true;
    }
    catch (...)
    {
        return false;
    }
    return false;
}

// 통계 6개를 한 번에 비교한다. 실패하면 어느 칸이 틀렸는지 먼저 출력하고 assert로 멈춘다
static void expect_stats(const Stats& s, uint64_t accesses, uint64_t hits, uint64_t misses,
                         uint64_t reads, uint64_t writes, uint64_t writebacks) {
    bool ok = s.accesses == accesses && s.hits == hits && s.misses == misses
              && s.reads == reads && s.writes == writes && s.writebacks == writebacks;
    if (!ok)
    {
        std::cerr << "  stats mismatch (got / expected)\n"
                  << "    accesses   " << s.accesses   << " / " << accesses   << "\n"
                  << "    hits       " << s.hits       << " / " << hits       << "\n"
                  << "    misses     " << s.misses     << " / " << misses     << "\n"
                  << "    reads      " << s.reads      << " / " << reads      << "\n"
                  << "    writes     " << s.writes     << " / " << writes     << "\n"
                  << "    writebacks " << s.writebacks << " / " << writebacks << "\n";
    }
    assert(ok);
}

constexpr bool READ  = false;
constexpr bool WRITE = true;


// ─────────────────────────────────────────────────────────────
// TODO(1): is_pow2
// ─────────────────────────────────────────────────────────────
static void test_pow2() {
    assert(is_pow2(1u));                    // 2^0
    assert(is_pow2(2u));
    assert(is_pow2(16u));                   // 0b1_0000: 1인 비트가 하나
    assert(is_pow2(1024u));
    assert(!is_pow2(0u));                   // 0은 2의 거듭제곱이 아니다 (경계 조건)
    assert(!is_pow2(3u));                   // 0b11
    assert(!is_pow2(12u));                  // 0b1100: 1인 비트가 둘
    assert(!is_pow2(255u));

    assert(is_pow2(uint64_t{1} << 63));     // 64비트 최상위 비트
    assert(!is_pow2((uint64_t{1} << 63) + 1));
    assert(is_pow2(uint64_t{32768}));

    assert(is_pow2(8));                     // T = int
    assert(!is_pow2(0));
    assert(!is_pow2(-8));                   // 음수는 false
    assert(!is_pow2(-1));                   // -1은 2진수로 1이 가득 → 따로 처리해야 한다
}


// ─────────────────────────────────────────────────────────────
// TODO(2): 생성자
// ─────────────────────────────────────────────────────────────
static void test_ctor() {
    // 256B, assoc 1, block 16
    //   numSets = 256 / (1 * 16) = 16, offsetBits = log2(16) = 4, indexBits = log2(16) = 4
    {
        Cache c(256, 1, 16);
        assert(CacheTester::numSets(c) == 16);
        assert(CacheTester::offsetBits(c) == 4);
        assert(CacheTester::indexBits(c) == 4);
        assert(CacheTester::lineCount(c) == 16);      // 16 sets * 1 way
    }
    // 256B, assoc 2, block 16
    //   numSets = 256 / (2 * 16) = 8, offsetBits = 4, indexBits = log2(8) = 3
    {
        Cache c(256, 2, 16);
        assert(CacheTester::numSets(c) == 8);
        assert(CacheTester::offsetBits(c) == 4);
        assert(CacheTester::indexBits(c) == 3);
        assert(CacheTester::lineCount(c) == 16);      // 8 sets * 2 ways
    }
    // 256B, assoc 16, block 16 = fully associative
    //   numSets = 256 / (16 * 16) = 1, indexBits = log2(1) = 0
    {
        Cache c(256, 16, 16);
        assert(CacheTester::numSets(c) == 1);
        assert(CacheTester::offsetBits(c) == 4);
        assert(CacheTester::indexBits(c) == 0);
        assert(CacheTester::lineCount(c) == 16);
    }
    // 32KB, assoc 8, block 64 (흔한 L1D 설정)
    //   numSets = 32768 / (8 * 64) = 64, offsetBits = log2(64) = 6, indexBits = log2(64) = 6
    {
        Cache c(32768, 8, 64);
        assert(CacheTester::numSets(c) == 64);
        assert(CacheTester::offsetBits(c) == 6);
        assert(CacheTester::indexBits(c) == 6);
        assert(CacheTester::lineCount(c) == 512);
    }
    // block 1: offsetBits = 0
    {
        Cache c(16, 1, 1);
        assert(CacheTester::numSets(c) == 16);
        assert(CacheTester::offsetBits(c) == 0);
        assert(CacheTester::indexBits(c) == 4);
    }

    // 잘못된 파라미터 → std::invalid_argument
    assert(throws_invalid_argument([] { Cache c(300, 1, 16); }));   // size가 2의 거듭제곱 아님
    assert(throws_invalid_argument([] { Cache c(256, 3, 16); }));   // assoc가 2의 거듭제곱 아님
    assert(throws_invalid_argument([] { Cache c(256, 1, 12); }));   // block이 2의 거듭제곱 아님
    assert(throws_invalid_argument([] { Cache c(0, 1, 16); }));     // 0은 2의 거듭제곱 아님
    assert(throws_invalid_argument([] { Cache c(256, 0, 16); }));
    assert(throws_invalid_argument([] { Cache c(256, 1, 0); }));
    assert(throws_invalid_argument([] { Cache c(64, 8, 16); }));    // 8 * 16 = 128 > 64: 안 나눠짐
    assert(throws_invalid_argument([] { Cache c(16, 1, 32); }));    // block이 size보다 큼

    // 경계: size == assoc * block 은 정상 (set 하나)
    assert(!throws_invalid_argument([] { Cache c(128, 8, 16); }));
}


// ─────────────────────────────────────────────────────────────
// TODO(3): 주소 분해
// ─────────────────────────────────────────────────────────────
static void test_addr() {
    // A: 256B, assoc 1, block 16 → offsetBits 4, indexBits 4
    //    | tag (나머지) | index (4비트) | offset (4비트) |
    {
        Cache c(256, 1, 16);
        // 0x1234 = 0001 0010 0011 0100
        //   offset = 하위 4비트           = 0100           = 0x4
        //   index  = 그 위 4비트          = 0011           = 0x3
        //   tag    = 0x1234 >> (4 + 4)   = 0001 0010      = 0x12
        assert(CacheTester::offsetOf(c, 0x1234) == 0x4);
        assert(CacheTester::indexOf(c, 0x1234) == 0x3);
        assert(CacheTester::tagOf(c, 0x1234) == 0x12);

        // 0xABCD: offset = 0xD, index = 0xC, tag = 0xAB (16진수 한 자리 = 4비트라 바로 읽힌다)
        assert(CacheTester::offsetOf(c, 0xABCD) == 0xD);
        assert(CacheTester::indexOf(c, 0xABCD) == 0xC);
        assert(CacheTester::tagOf(c, 0xABCD) == 0xAB);

        // 0x00F: 블록 0의 마지막 바이트 → offset 0xF, index 0, tag 0
        assert(CacheTester::offsetOf(c, 0x00F) == 0xF);
        assert(CacheTester::indexOf(c, 0x00F) == 0x0);
        assert(CacheTester::tagOf(c, 0x00F) == 0x0);

        // 0x100: index는 0x000과 같고(0), tag만 다르다(1) → v1에서 충돌 miss가 나는 쌍
        assert(CacheTester::offsetOf(c, 0x100) == 0x0);
        assert(CacheTester::indexOf(c, 0x100) == 0x0);
        assert(CacheTester::tagOf(c, 0x100) == 0x1);
    }
    // B: 256B, assoc 2, block 16 → offsetBits 4, indexBits 3 (16진수 자리와 경계가 안 맞는다)
    {
        Cache c(256, 2, 16);
        // 0x1234 = 0001 0010 0011 0100
        //   offset = 하위 4비트                    = 0100              = 0x4
        //   index  = (0x1234 >> 4) & 0b111 = 0x123 & 7, 0x123 = ...0010 0011 → 011 = 3
        //   tag    = 0x1234 >> (4 + 3) = 0001 0010 0011 0100 >> 7 = 0 0010 0100 = 0x24
        assert(CacheTester::offsetOf(c, 0x1234) == 0x4);
        assert(CacheTester::indexOf(c, 0x1234) == 3);
        assert(CacheTester::tagOf(c, 0x1234) == 0x24);

        // 0xABCD = 1010 1011 1100 1101
        //   offset = 1101 = 0xD
        //   index  = 0xABC & 7, 0xABC = ...1011 1100 → 100 = 4
        //   tag    = 0xABCD >> 7 = 1 0101 0111 = 0x157
        assert(CacheTester::offsetOf(c, 0xABCD) == 0xD);
        assert(CacheTester::indexOf(c, 0xABCD) == 4);
        assert(CacheTester::tagOf(c, 0xABCD) == 0x157);

        // 0x080 = 1000 0000: offset 0, index (0x08 & 7) = 0, tag 0x080 >> 7 = 1
        //   → set 0에 들어가는 블록은 0x000, 0x080, 0x100, 0x180, ... (128바이트 간격, tag 0, 1, 2, 3)
        assert(CacheTester::indexOf(c, 0x080) == 0);
        assert(CacheTester::tagOf(c, 0x080) == 1);
        assert(CacheTester::indexOf(c, 0x100) == 0);
        assert(CacheTester::tagOf(c, 0x100) == 2);
    }
    // C: 256B, assoc 16, block 16 = fully associative → indexBits 0
    {
        Cache c(256, 16, 16);
        // set이 하나뿐이므로 index는 항상 0, tag = addr >> 4
        assert(CacheTester::offsetOf(c, 0xABCD) == 0xD);
        assert(CacheTester::indexOf(c, 0xABCD) == 0);
        assert(CacheTester::tagOf(c, 0xABCD) == 0xABC);
    }
    // D: 큰 주소 (64비트 상위 비트까지 tag에 남는가)
    {
        Cache c(32768, 8, 64);   // offsetBits 6, indexBits 6
        const uint64_t addr = 0xFFFF'0000'0000'1FC5;
        // 하위 12비트 0x FC5 = 1111 1100 0101
        //   offset = 하위 6비트 = 00 0101 = 0x05
        //   index  = 그 위 6비트 = 1111 11 = 0x3F
        //   tag    = addr >> 12 = 0xFFFF'0000'0000'1
        assert(CacheTester::offsetOf(c, addr) == 0x05);
        assert(CacheTester::indexOf(c, addr) == 0x3F);
        assert(CacheTester::tagOf(c, addr) == 0xF'FFF0'0000'0001);
    }
}


// ─────────────────────────────────────────────────────────────
// TODO(4): v1 direct-mapped
// ─────────────────────────────────────────────────────────────
static void test_v1() {
    // A: 256B, assoc 1, block 16 (16 sets). 0x000과 0x100은 둘 다 set 0 (tag 0, tag 1)
    {
        Cache c(256, 1, 16);
        assert(c.stats().missRate() == 0.0);     // 접근 전: 0으로 나누지 않는다
        assert(!c.access(0x000, READ));  // 1. set 0 비어 있음 → cold miss,    set 0 = tag 0
        assert(c.access(0x004, READ));   // 2. 같은 블록(0x000–0x00F) → hit
        assert(!c.access(0x100, READ));  // 3. set 0, tag 1 ≠ 0 → miss,         set 0 = tag 1 (tag 0 쫓겨남)
        assert(!c.access(0x000, READ));  // 4. set 0, tag 0 ≠ 1 → 충돌(conflict) miss, set 0 = tag 0
        assert(!c.access(0x010, READ));  // 5. set 1 비어 있음 → cold miss
        assert(c.access(0x01C, READ));   // 6. 0x010–0x01F 블록 → hit
        assert(!c.access(0x100, READ));  // 7. set 0, tag 1 ≠ 0 → 충돌 miss
        //                 accesses hits misses reads writes writebacks
        expect_stats(c.stats(), 7,   2,    5,     7,    0,     0);
        // missRate = 5 / 7
        assert(std::fabs(c.stats().missRate() - 5.0 / 7.0) < 1e-12);
    }
    // 쓰기도 hit/miss 판정은 같다. writes만 센다 (writebacks는 TODO(6)에서 확인)
    {
        Cache c(256, 1, 16);
        assert(!c.access(0x0A0, WRITE)); // 1. set 0xA cold miss (write-allocate: 블록을 채운다)
        assert(c.access(0x0A8, READ));   // 2. 같은 블록 → hit (쓰기 miss도 채웠으므로)
        assert(c.access(0x0AF, WRITE));  // 3. 같은 블록 → hit
        assert(!c.access(0x1A0, READ));  // 4. set 0xA, tag 1 ≠ 0 → miss
        const Stats& s = c.stats();
        assert(s.accesses == 4 && s.hits == 2 && s.misses == 2);
        assert(s.reads == 2 && s.writes == 2);
    }
}


// ─────────────────────────────────────────────────────────────
// TODO(5): v2 LRU
// ─────────────────────────────────────────────────────────────
static void test_lru() {
    // B: 256B, assoc 2, block 16 (8 sets). set 0에 들어가는 블록: 0x000(A) 0x080(B) 0x100(C) 0x180(D)
    //
    // 1) v1과 같은 시퀀스: 2-way면 0x000과 0x100이 set 0에 같이 있을 수 있다 → 충돌 miss가 사라진다
    //    (invalid way를 먼저 채우는지 확인)
    {
        Cache c(256, 2, 16);
        assert(!c.access(0x000, READ));  // 1. cold miss        set 0 = [A, -]
        assert(c.access(0x004, READ));   // 2. hit
        assert(!c.access(0x100, READ));  // 3. cold miss        set 0 = [A, C]  (빈 way에 채움, A는 남는다)
        assert(c.access(0x000, READ));   // 4. hit  (v1에서는 miss였다)
        assert(!c.access(0x010, READ));  // 5. set 1 cold miss
        assert(c.access(0x01C, READ));   // 6. hit
        assert(c.access(0x100, READ));   // 7. hit  (v1에서는 miss였다)
        expect_stats(c.stats(), 7, 4, 3, 7, 0, 0);
    }
    // 2) LRU 교체 순서. 괄호 안은 (way 0, way 1), *는 LRU(다음에 쫓겨날 라인)
    //    FIFO(먼저 들어온 것)나 "항상 way 0"으로 구현하면 4번 또는 5번에서 틀린다
    {
        Cache c(256, 2, 16);
        assert(!c.access(0x000, READ));  // 1. A miss            (A, -)
        assert(!c.access(0x080, READ));  // 2. B miss            (A*, B)
        assert(c.access(0x000, READ));   // 3. A hit             (A, B*)   A를 쓴 뒤 LRU는 B
        assert(!c.access(0x100, READ));  // 4. C miss, B 쫓겨남   (A*, C)   FIFO라면 A가 쫓겨난다
        assert(c.access(0x000, READ));   // 5. A hit             (A, C*)
        assert(!c.access(0x080, READ));  // 6. B miss, C 쫓겨남   (A*, B)
        assert(!c.access(0x100, READ));  // 7. C miss, A 쫓겨남   (C, B*)
        assert(c.access(0x080, READ));   // 8. B hit             (C*, B)
        //                 accesses hits misses reads writes writebacks
        expect_stats(c.stats(), 8,   3,    5,     8,    0,     0);
    }
    // 3) 4-way fully associative(64B, block 16 → set 1개). 5개 블록을 돌면 LRU는 매번 miss
    {
        Cache c(64, 4, 16);
        const std::vector<uint64_t> blocks = {0x00, 0x10, 0x20, 0x30, 0x40};
        for (int round = 0; round < 2; ++round)
        {
            for (uint64_t addr : blocks)
            {
                // 1회차: cold miss 5번 (5번째에서 0x00이 쫓겨남)
                // 2회차: 매번 "방금 필요한 블록"이 LRU로 쫓겨난 상태 → 전부 miss
                assert(!c.access(addr, READ));
            }
        }
        expect_stats(c.stats(), 10, 0, 10, 10, 0, 0);
    }
}


// ─────────────────────────────────────────────────────────────
// TODO(6): v2 write-back + dirty
// ─────────────────────────────────────────────────────────────
static void test_writeback() {
    // 1) direct-mapped. 0x000과 0x100은 set 0에서 충돌
    {
        Cache c(256, 1, 16);
        c.access(0x000, WRITE);  // 1. miss, 채움, dirty
        c.access(0x100, READ);   // 2. miss, victim(tag 0)이 dirty → writeback 1
        c.access(0x000, READ);   // 3. miss, victim(tag 1)은 clean → writeback 없음
        c.access(0x100, READ);   // 4. miss, victim(tag 0)은 3번에서 읽기로 다시 채워 clean → 없음
        expect_stats(c.stats(), 4, 0, 4, 3, 1, 1);
    }
    // 2) 2-way LRU + dirty. tests/sample.txt와 같은 시퀀스 (README의 ./sim 확인에서도 쓴다)
    //    (way 0, way 1), *는 LRU, '는 dirty
    {
        Cache c(256, 2, 16);
        assert(!c.access(0x000, WRITE)); // 1. A miss, dirty                  (A', -)
        assert(!c.access(0x080, READ));  // 2. B miss                         (A'*, B)
        assert(!c.access(0x100, READ));  // 3. C miss, A' 쫓겨남 → wb 1        (C, B*)
        assert(c.access(0x080, WRITE));  // 4. B hit, dirty                   (C*, B')
        assert(!c.access(0x180, READ));  // 5. D miss, C(clean) 쫓겨남 → wb 1  (D, B'*)
        assert(!c.access(0x000, READ));  // 6. A miss, B' 쫓겨남 → wb 2        (D*, A)
        assert(c.access(0x000, WRITE));  // 7. A hit, dirty                   (D*, A')
        assert(c.access(0x004, WRITE));  // 8. A hit (같은 블록), 이미 dirty    (D*, A')
        //                 accesses hits misses reads writes writebacks
        expect_stats(c.stats(), 8,   3,    5,     4,    4,     2);
        // 끝에 A'가 dirty로 남아 있지만 flush는 하지 않으므로 writebacks는 2 그대로
    }
    // 3) 같은 블록에 여러 번 써도 쫓겨날 때 writeback은 한 번
    {
        Cache c(256, 1, 16);
        c.access(0x020, WRITE);  // miss, dirty
        c.access(0x024, WRITE);  // hit
        c.access(0x02C, WRITE);  // hit
        c.access(0x120, READ);   // miss, set 2 victim dirty → wb 1
        expect_stats(c.stats(), 4, 2, 2, 1, 3, 1);
    }
}


// ─────────────────────────────────────────────────────────────
// load_trace (완성됨)
// ─────────────────────────────────────────────────────────────
static void test_trace() {
    // tests/sample.txt: 주석·빈 줄·줄 끝 주석이 섞인 8줄
    const std::vector<Access> t = load_trace("tests/sample.txt");
    assert(t.size() == 8);
    assert(t[0].isWrite && t[0].addr == 0x000);
    assert(!t[1].isWrite && t[1].addr == 0x080);
    assert(!t[2].isWrite && t[2].addr == 0x100);
    assert(t[3].isWrite && t[3].addr == 0x080);
    assert(t[7].isWrite && t[7].addr == 0x004);

    // 한 줄 파싱
    Access a = parse_trace_line("W 0x1a2b");
    assert(a.isWrite && a.addr == 0x1a2b);
    a = parse_trace_line("  R   0XFFFF  ");
    assert(!a.isWrite && a.addr == 0xFFFF);
    assert(throws_invalid_argument([] { parse_trace_line("X 0x10"); }));     // R/W가 아님
    assert(throws_invalid_argument([] { parse_trace_line("R"); }));          // 주소 없음
    assert(throws_invalid_argument([] { parse_trace_line("R zz"); }));       // 16진수 아님
    assert(throws_invalid_argument([] { parse_trace_line("R 0x10 5"); }));   // 남는 글자

    // 파일 단위 오류: tests/bad_line.txt 3번째 줄이 잘못됨 → 메시지에 "파일:3:"
    bool caught = false;
    try
    {
        load_trace("tests/bad_line.txt");
    }
    catch (const std::invalid_argument& e)
    {
        caught = std::string(e.what()).find("tests/bad_line.txt:3:") != std::string::npos;
    }
    assert(caught);

    // 없는 파일 → std::runtime_error
    caught = false;
    try
    {
        load_trace("tests/no_such_file.txt");
    }
    catch (const std::runtime_error&)
    {
        caught = true;
    }
    assert(caught);
}


// ─────────────────────────────────────────────────────────────
// main: 단계 이름 → 테스트 함수
// ─────────────────────────────────────────────────────────────
struct Stage {
    std::string name;
    void (*run)();
};

const std::vector<Stage> STAGES = {
    {"pow2",      test_pow2},
    {"ctor",      test_ctor},
    {"addr",      test_addr},
    {"v1",        test_v1},
    {"lru",       test_lru},
    {"writeback", test_writeback},
    {"trace",     test_trace},
};

static bool run_stage(const std::string& name) {
    for (const Stage& s : STAGES)
    {
        if (s.name == name)
        {
            std::cout << "[" << s.name << "] ";
            std::cout.flush();          // assert로 멈춰도 어느 단계였는지 보이게
            s.run();
            std::cout << "passed\n";
            return true;
        }
    }
    return false;
}

static void print_usage(std::ostream& os, const char* prog) {
    os << "usage: " << prog << " <stage>\n  stages:";
    for (const Stage& s : STAGES)
    {
        os << " " << s.name;
    }
    os << " v2 all\n";
}

int main(int argc, char* argv[]) {
    if (argc != 2)
    {
        print_usage(std::cerr, argv[0]);
        return 1;
    }
    const std::string stage = argv[1];

    if (stage == "all")
    {
        for (const Stage& s : STAGES)
        {
            run_stage(s.name);
        }
        std::cout << "all tests passed\n";
        return 0;
    }
    if (stage == "v2")
    {
        run_stage("lru");
        run_stage("writeback");
        return 0;
    }
    if (!run_stage(stage))
    {
        std::cerr << "unknown stage '" << stage << "'\n";
        print_usage(std::cerr, argv[0]);
        return 1;
    }
    return 0;
}
