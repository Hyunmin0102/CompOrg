// W6: 메모리 접근 trace 읽기
//   trace 형식: 한 줄에 "R 0x1a2b" 또는 "W 0x1a2b". '#' 이후는 주석, 빈 줄 허용
#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct Access {
    bool     isWrite = false;   // 'W'이면 true, 'R'이면 false
    uint64_t addr    = 0;
};

// trace 한 줄("R 0x1a2b")을 Access로 바꾼다. 잘못된 줄이면 std::invalid_argument
Access parse_trace_line(const std::string& line);

// trace 파일 전체를 읽는다. 파일을 못 열면 std::runtime_error,
// 파싱 오류는 "파일:줄번호: "를 앞에 붙여 std::invalid_argument로 다시 던진다.
std::vector<Access> load_trace(const std::string& path);
