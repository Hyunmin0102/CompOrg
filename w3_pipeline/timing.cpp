#include "timing.h"

#include <iomanip>
#include <algorithm>
// 스켈레톤 규칙 (W2와 같음)
//  - 반환값이 있는 함수의 `return ...;`은 -Wreturn-type 경고를 막는 임시 값이다. 구현하면서 교체한다.
//  - `(void)x;`는 -Wunused-parameter 경고를 막는 임시 줄이다. 매개변수를 쓰기 시작하면 삭제한다.

double Stats::cpi() const {
    // TODO(W3): cycles / insts. 정수끼리 나누면 소수점이 잘린다 → static_cast<double>.
    //           insts == 0 (빈 프로그램)이면 무엇을 돌려줄지도 정한다.
    return 0.0;
}

void printStats(std::ostream& os, const Stats& s, bool forwarding) {
    // TODO(W3): forwarding on/off, 명령어 수, 총 cycle, stall(그중 load-use), taken beq, 분기 손실, CPI 출력.
    //           CPI는 std::fixed << std::setprecision(3). 둘 다 끈적(sticky)하니 출력 뒤 되돌릴지 생각해 본다.
    (void)os;
    (void)s;
    (void)forwarding;
}


// ─────────────────────────────────────────────────────────────
// 레지스터 사용 정보
// ─────────────────────────────────────────────────────────────
std::optional<uint8_t> destReg(const DecodedInst& d) {
    uint8_t dest_reg = 0; //zero를 따로 다뤄야 하는 상황이면 애매할수도
    if((d.opcode == OP_R) && (d.funct == FUNCT_ADD))
    {
        dest_reg = d.rd;
    }
    else if((d.opcode == OP_ADDI) || (d.opcode == OP_LW))
    {
        dest_reg = d.rt;
    }
    if(dest_reg != 0)
    {
        return dest_reg;
    }
    return std::nullopt;
}

bool readsReg(const DecodedInst& d, uint8_t r) {
    switch(d.opcode)
    {
        case OP_R:
            if(d.funct == FUNCT_ADD) return d.rs == r || d.rt == r;
            break;
        case OP_SW: 
        case OP_BEQ: return d.rs == r || d.rt == r;
        case OP_ADDI:
        case OP_LW: return d.rs == r;
        default: return false;
    }
    return false;
}


// ─────────────────────────────────────────────────────────────
// PipelineTimer
// ─────────────────────────────────────────────────────────────
PipelineTimer::PipelineTimer() : forwarding_(true), stats_{}, prev1_{}, prev2_{} {
}

void PipelineTimer::setForwarding(bool on) {
    forwarding_ = on;
}

bool PipelineTimer::forwarding() const {
    return forwarding_;
}

uint64_t PipelineTimer::idEnterCycle() const {
    //           insts + stalls + branchPenalty + 2  (2 = 첫 명령어의 IF 1 cycle + 이번 명령어 몫 1 cycle)
    return stats_.insts + stats_.stalls + stats_.branchPenalty + 2;
}

uint64_t PipelineTimer::stallsFrom(const DecodedInst& d, uint64_t enter, const InstRecord& p) const {
    if(p.dest.has_value() && readsReg(d, *(p.dest)))
    {
        uint64_t real_dist = enter > p.issueCycle ? enter - p.issueCycle : 0;  // 정상이면 항상 enter > p.issueCycle
        uint64_t need_dist = 0;
        if(!forwarding_)
        {
            need_dist = 3; // forwarding off 이면, 무조건 거리 3 이상 떨어져있어야함 필요
        }
        else if(p.isLoad)
        {
            need_dist = 2; // forwarding on이면서 load일 때 : 거리 2 이상 떨어져있어야함
        }
        else
        {
            need_dist = 1; // forwarding on이면서 load가 아닐 때 : 거리 1 이상 떨어져있어야함.
        }
        uint64_t req_stall = real_dist < need_dist ? need_dist - real_dist : 0;
        return req_stall;
    }
    return 0;
}


uint64_t PipelineTimer::stallsFor(const DecodedInst& d, uint64_t enter) const {
    uint64_t fromPrev1 = 0;
    uint64_t fromPrev2 = 0;
    if(prev1_.has_value())
    {
        fromPrev1 = stallsFrom(d, enter, *(prev1_));
    }
    if(prev2_.has_value())
    {
        fromPrev2 = stallsFrom(d, enter, *(prev2_));
    }
    return std::max(fromPrev1, fromPrev2);
}

PipelineTimer::InstRecord PipelineTimer::makeRecord(const DecodedInst& d, uint64_t issueCycle) const {
    std::optional<uint8_t> dest = destReg(d);
    bool isLoad = (d.opcode == OP_LW);
    uint64_t issue_cycle = issueCycle;
    InstRecord inst = InstRecord{dest, isLoad, issue_cycle};
    return inst;
}

void PipelineTimer::onInst(const DecodedInst& d, bool branchTaken) {
    uint64_t enter = idEnterCycle();
    uint64_t stall = stallsFor(d, enter);
    InstRecord rec = makeRecord(d, enter + stall);
    stats_.stalls += stall;
    stats_.insts += 1;
    if(branchTaken)
    {
        stats_.takenBranches += 1;
        stats_.branchPenalty += BRANCH_PENALTY;
    }
    stats_.cycles = stats_.insts + stats_.stalls + PIPELINE_FILL + stats_.branchPenalty;
    if(forwarding_)
    {
        stats_.loadUseStalls += stall;
    }
    prev2_ = prev1_;
    prev1_ = rec;
}

Stats PipelineTimer::stats() const {
    return stats_;
}
