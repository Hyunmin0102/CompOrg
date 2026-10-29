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
    (void)d;
    (void)r;
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

PipelineTimer::InstRecord PipelineTimer::makeRecord(const DecodedInst& d) const {
    // TODO(W3): destReg()와 opcode로 InstRecord를 채운다.
    (void)d;
    return InstRecord{};
}

uint64_t PipelineTimer::stallsFor(const DecodedInst& d) const {
    // TODO(W3): prev1_ / prev2_가 있으면(has_value) 그 dest를 d가 읽는지 본다.
    //           forwarding_에 따라 거리별 stall 수가 다르다 (timing.h 맨 위 가정).
    //           두 생산자 모두와 의존하면 stall을 더하는가, 큰 쪽만 쓰는가?
    (void)d;
    return 0;
}

void PipelineTimer::onInst(const DecodedInst& d, bool branchTaken) {
    // TODO(W3): 1) stallsFor(d)로 stall을 구해 stats_.stalls(필요하면 loadUseStalls)에 더한다
    //           2) insts 증가, branchTaken이면 takenBranches·branchPenalty 증가
    //           3) 기록을 한 칸씩 민다: prev2_ ← prev1_, prev1_ ← makeRecord(d)
    //              (순서를 바꾸면 무엇이 사라지는지 생각해 본다)
    (void)d;
    (void)branchTaken;
}

Stats PipelineTimer::stats() const {
    // TODO(W3): stats_를 복사한 뒤 cycles를 채워 돌려준다.
    //           PIPELINE_FILL을 여기서 더하면 stats()를 몇 번 불러도 결과가 같다.
    //           명령어가 0개일 때 cycles는 몇이어야 하는가?
    return stats_;
}
