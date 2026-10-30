#include "timing.h"

#include <iomanip>

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
    // TODO(W3): 명령어마다 실제로 읽는 소스만 비교한다.
    //           add·sw·beq는 rs와 rt, addi·lw는 rs만. (sw의 rt = 저장할 값도 소스)

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
    // TODO(W3): stats_가 아직 이번 명령어를 반영하기 전의 값으로 계산한다.
    //           insts + stalls + branchPenalty + 2  (2 = 첫 명령어의 IF 1 cycle + 이번 명령어 몫 1 cycle)
    return 0;
}

uint64_t PipelineTimer::stallsFor(const DecodedInst& d, uint64_t enter) const {
    // TODO(W3): 앞 명령어 p(prev1_, prev2_)마다
    //             p가 있고, p->dest가 있고, d가 *(p->dest)를 읽으면
    //               실제 거리 = enter - p->issueCycle
    //               필요 거리 = forwarding off → 3, on + p가 lw → 2, on + lw 아님 → 1
    //               요구 stall = 실제 거리 < 필요 거리 ? 필요 거리 - 실제 거리 : 0   (부호 없는 뺄셈 주의)
    //           두 요구 stall 중 큰 쪽을 돌려준다.
    //           같은 계산이 두 번 나오면 생산자 하나를 받는 헬퍼로 빼도 된다.
    (void)d;
    (void)enter;
    return 0;
}

PipelineTimer::InstRecord PipelineTimer::makeRecord(const DecodedInst& d, uint64_t issueCycle) const {
    // TODO(W3): dest = destReg(d), isLoad = (d가 lw인가), issueCycle = 받은 값
    (void)d;
    (void)issueCycle;
    return InstRecord{};
}

void PipelineTimer::onInst(const DecodedInst& d, bool branchTaken) {
    // TODO(W3): ① enter = idEnterCycle()          (stats_를 갱신하기 전에!)
    //           ② stall = stallsFor(d, enter)
    //           ③ rec   = makeRecord(d, enter + stall)
    //           ④ stats_ 갱신: stalls(필요하면 loadUseStalls), insts, branchTaken이면 takenBranches·branchPenalty
    //           ⑤ prev2_ ← prev1_, prev1_ ← rec   (순서를 바꾸면 무엇이 사라지는지 생각해 본다)
    (void)d;
    (void)branchTaken;
}

Stats PipelineTimer::stats() const {
    // TODO(W3): stats_를 복사한 뒤 cycles를 채워 돌려준다.
    //           PIPELINE_FILL을 여기서 더하면 stats()를 몇 번 불러도 결과가 같다.
    //           명령어가 0개일 때 cycles는 몇이어야 하는가?
    return stats_;
}
