// W3: 5단 파이프라인(IF/ID/EX/MEM/WB) 타이밍 통계
// 기능 시뮬레이션(Cpu)은 그대로 두고, 실행된 명령어 순서만 보고 stall과 cycle을 센다.
//
// 파이프라인 가정 (L12 H&H 5단 기준)
//   - 레지스터 파일: WB 앞 반 사이클에 쓰고 ID 뒤 반 사이클에 읽는다
//   - forwarding on : M→E, W→E 경로. load-use(lw 직후 사용, sw의 rt 포함) stall 1, 그 외 0
//   - forwarding off: 의존 거리 1이면 stall 2, 거리 2면 stall 1, 거리 3 이상이면 0
//   - beq: always-not-taken, MEM에서 결정, taken이면 3 cycle flush.
//          beq도 rs·rt를 읽는 일반 소비자로 hazard를 판정한다
//   - 총 cycle = 명령어 수 + 4 + stall 수 + 3 × (taken된 beq 수)
//   - 거리: 바로 다음 명령어 = 거리 1
//     거리는 명령어 순서가 아니라 실제 cycle 기준이다. 앞선 stall과 분기 flush로 흐른 cycle도
//     생산자와 소비자 사이의 거리에 포함된다.
//     예) dist2.txt --no-forwarding: [0x14] add가 stall하면 [0x10] addi → [0x18] sw의 거리도 늘어난다
//         sum1to10.txt: addi $t0 → (taken beq, flush 3) → 목적지 beq $t0의 거리는 2보다 크다
//   - hazard 판정은 "실제로 읽는" 소스 레지스터만 본다.
//     슬라이드 69의 lwstall 식은 (rsD == rtE) OR (rtD == rtE)로 rt를 항상 비교한다
//     (하드웨어는 명령어 종류를 따지기 전에 비교하므로 보수적). 여기서는 addi·lw처럼
//     rt가 목적지인 명령어의 rt는 소스로 보지 않는다 → 슬라이드보다 stall이 적게 나올 수 있다.
//   - $zero를 목적지로 쓰는 명령어는 의존을 만들지 않는다
#ifndef TIMING_H
#define TIMING_H

#include <cstdint>
#include <optional>
#include <ostream>

#include "decoder.h"

// 가정에서 나온 상수. 바꿀 때 이곳만 고친다.
constexpr uint64_t PIPELINE_FILL  = 4;   // 첫 명령어가 WB에 도달하기까지 추가로 드는 cycle (5단 - 1)
constexpr uint64_t BRANCH_PENALTY = 3;   // taken beq 하나당 flush되는 cycle (MEM에서 결정)

// ─────────────────────────────────────────────────────────────
// 통계
//   기본 멤버 초기화(= 0) 덕분에 Stats s; 만 써도 모든 카운터가 0에서 시작한다.
// ─────────────────────────────────────────────────────────────
struct Stats {
    uint64_t insts         = 0;  // 완료한 명령어 수
    uint64_t cycles        = 0;  // 총 cycle = insts + PIPELINE_FILL + stalls + branchPenalty
    uint64_t stalls        = 0;  // 데이터 hazard로 생긴 stall cycle 전체
    uint64_t loadUseStalls = 0;  // forwarding on일 때만 센다 (on에서는 lw만 stall을 만들므로 = stalls). off면 0
    uint64_t takenBranches = 0;  // taken된 beq 수
    uint64_t branchPenalty = 0;  // 분기 flush로 잃은 cycle (stalls와 따로 센다)

    double cpi() const;          // cycles / insts
};

// 통계 출력 (std::fixed, std::setprecision(3) 연습 자리)
void printStats(std::ostream& os, const Stats& s, bool forwarding);


// ─────────────────────────────────────────────────────────────
// 타이밍 모델
//   Cpu::step()이 명령어 하나를 실행할 때마다 onInst()를 부른다.
//   Cpu는 "무엇을 했는가"만, PipelineTimer는 "몇 cycle 걸렸는가"만 안다.
// ─────────────────────────────────────────────────────────────
class PipelineTimer {
public:
    PipelineTimer();

    void  setForwarding(bool on);
    bool  forwarding() const;

    // 방금 실행을 마친 명령어 하나를 통계에 반영한다.
    // branchTaken: beq가 실제로 분기했는가 (beq가 아니면 false)
    void  onInst(const DecodedInst& d, bool branchTaken);

    Stats stats() const;          // 지금까지의 통계 (총 cycle 포함)

private:
    // 앞서 실행된 명령어에서 hazard 판정에 필요한 정보만 남긴 기록
    struct InstRecord {
        std::optional<uint8_t> dest;      // 앞 명령어가 쓰는 레지스터. 이 값을 readsReg로 넘김. sw·beq처럼 안 쓰면 std::nullopt
        bool                   isLoad = false;  // 앞 명령어가 lw인지 여부. forwarding on 일 때 load-use stall을 판단하기 위해 추가함.
        uint64_t               issueCycle = 0;  // stall이 끝나고 ID를 떠난 cycle (다이어그램 cycle 번호와 같음)
    };

    // onInst()가 아래 순서로 부른다.
    //   enter = idEnterCycle()  →  stall = stallsFor(d, enter)  →  makeRecord(d, enter + stall)
    uint64_t   idEnterCycle() const;                                 // 이번 명령어가 stall 없이 ID에 들어갈 cycle
    uint64_t   stallsFor(const DecodedInst& d, uint64_t enter) const; // 이번 명령어가 ID에서 기다려야 하는 cycle 수
    InstRecord makeRecord(const DecodedInst& d, uint64_t issueCycle) const; // 이번 명령어의 기록 (다음 명령어들이 읽음)

    bool  forwarding_;
    Stats stats_;

    // 직전 명령어와 그 앞 명령어의 기록. 프로그램 시작 직후에는 "없음"
    // 세 칸 앞부터는 실제 거리가 3 이상이라 stall을 만들지 않으므로 두 칸이면 충분하다
    std::optional<InstRecord> prev1_;
    std::optional<InstRecord> prev2_;
};


// 디코드 결과에서 레지스터 사용 정보를 뽑는 헬퍼 (명령어 5개: add, addi, lw, sw, beq)
std::optional<uint8_t> destReg(const DecodedInst& d);       // 쓰는 레지스터 ($zero면?)
bool                   readsReg(const DecodedInst& d, uint8_t r); // r을 실제로 읽는가

#endif // TIMING_H
