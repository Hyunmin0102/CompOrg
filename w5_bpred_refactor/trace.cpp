#include "trace.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

// W4 bpred.cpp의 parseTraceLine, loadTrace를 그대로 옮겼다.

// trace 한 줄("0x400120 1")을 BranchRecord로 바꾼다.
// 호출하는 쪽(loadTrace)이 '#' 주석 줄과 빈 줄은 이미 걸러 낸다.
BranchRecord parseTraceLine(const std::string& line) {
    std::istringstream iss(line); // line을 읽어와 공백을 기준으로 나눔.

    BranchRecord br;
    iss >> std::hex >> br.pc >> std::dec >> br.taken; //hex 모드의 operator>>는 0x, 0X를 받아줌.
    if (iss.fail())
    {
        throw std::invalid_argument("Can't read two values correctly"); // taken 이 0 or 1이 아니거나 pc가 16진수가 아니면 -> invalid argument
    }
    iss >> std::ws;
    if (!iss.eof())
    {
        throw std::invalid_argument("Useless string left");
    }
    return br;
}

// trace 파일 전체를 읽는다. '#' 이후는 주석, 빈 줄 허용.
// 파싱 오류는 "파일:줄번호: " 를 앞에 붙여 다시 던진다.
std::vector<BranchRecord> loadTrace(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("cannot open trace file: " + path);
    }

    std::vector<BranchRecord> trace;
    std::string line;
    size_t lineNo = 0;
    while (std::getline(file, line))
    {
        lineNo++;
        std::string code = line.substr(0, line.find('#'));
        if (code.find_first_not_of(" \t\r") == std::string::npos)
        {
            continue;
        }
        try
        {
            trace.push_back(parseTraceLine(code));
        }
        catch (const std::exception& e)
        {
            throw std::invalid_argument(path + ":" + std::to_string(lineNo) + ": " + e.what());
        }
    }
    return trace;
}
