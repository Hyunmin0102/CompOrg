// W2: MIPS 기능 시뮬레이터 (functional simulator)
// step() 한 번 = 명령어 하나 완료 (단일 사이클 프로세서와 같은 동작)
#ifndef CPU_H
#define CPU_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "decoder.h"

class Cpu {
public:
    // imemWords / dmemWords: 명령어·데이터 메모리 크기 (워드 단위)
    Cpu(size_t imemWords, size_t dmemWords);

    void loadProgram(const std::vector<uint32_t>& words); // fetch, decode... 등의 단계를 거치기 전, OS의 로더가 하는 일. 실행 파일을 메모리에 올림.
    void step();                       // fetch → decode → execute, 명령어 1개 완료
    void run(size_t maxInsts);         // done()까지 step() 반복, maxInsts 초과 시 throw
    bool done() const;                 // PC가 프로그램 끝을 넘었는가

    uint32_t reg(int i) const;         // 레지스터 읽기
    uint32_t pc() const;               // 현재 PC (main의 catch에서 출력용)
    uint32_t loadWord(uint32_t addr) const;        // 데이터 메모리 읽기 (결과 확인·assert용)
    void     storeWord(uint32_t addr, uint32_t value); // 데이터 메모리 쓰기 (테스트 초기 데이터용)
    void     dumpRegs() const;         // 레지스터 덤프 (16진수/10진수)

private:
    uint32_t    fetch() const;                    // imem_에서 PC 위치의 명령어 읽기
    DecodedInst decodeInst(uint32_t raw) const;   // 전역 decode() 호출 (W3에서 hazard 검사 끼워 넣을 자리)
    void        execute(const DecodedInst& d);    // 레지스터·메모리·PC 갱신
    size_t      memIndex(uint32_t addr, size_t memWords) const; // 바이트 주소 → 워드 인덱스 (정렬·범위 검사)

    std::array<uint32_t, 32> regs_;    // 크기 고정 → array
    uint32_t                 pc_;      // pc 자체. 다음에 실행할 명령어의 주소
    std::vector<uint32_t>    imem_;    // 명령어가 담긴 메모리 공간
    std::vector<uint32_t>    dmem_;    // 데이터가 담긴 메모리 공간
    size_t                   programWords_; // 적재된 프로그램 길이 (총 몇 개의 명령어가 있는지)
};

#endif // CPU_H
