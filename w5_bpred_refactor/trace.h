// W5: 분기 trace 읽기 (W4 bpred.cpp에서 그대로 옮김)
//   trace 형식: 한 줄에 "PC(16진수) taken(0/1)", '#' 이후는 주석, 빈 줄 허용
#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct BranchRecord {
    uint64_t pc    = 0;
    bool     taken = false;
};

// trace 한 줄("0x400120 1")을 BranchRecord로 바꾼다. 잘못된 줄이면 std::invalid_argument
BranchRecord parseTraceLine(const std::string& line);

// trace 파일 전체를 읽는다. 파싱 오류는 "파일:줄번호: "를 앞에 붙여 다시 던진다.
std::vector<BranchRecord> loadTrace(const std::string& path);
