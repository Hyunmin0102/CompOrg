#include "trace.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

// W5 trace.cpp(parseTraceLine, loadTrace)와 같은 구조. 한 줄의 형식만 바뀌었다.

// 호출하는 쪽(load_trace)이 '#' 주석과 빈 줄은 이미 걸러 낸다.
Access parse_trace_line(const std::string& line) {
    std::istringstream iss(line);

    char op = 0;
    uint64_t addr = 0;
    iss >> op >> std::hex >> addr;   // op: 공백을 건너뛴 첫 글자, addr: hex 모드는 "0x" 접두사를 받아 준다
    if (iss.fail())
    {
        throw std::invalid_argument("expected '<R|W> <hex address>', got '" + line + "'");
    }
    if (op != 'R' && op != 'W')
    {
        throw std::invalid_argument(std::string("access type must be R or W, got '") + op + "'");
    }
    iss >> std::ws;
    if (!iss.eof())
    {
        throw std::invalid_argument("unexpected text after address in '" + line + "'");
    }

    Access a;
    a.isWrite = (op == 'W');
    a.addr    = addr;
    return a;
}

std::vector<Access> load_trace(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("cannot open trace file: " + path);
    }

    std::vector<Access> trace;
    std::string line;
    size_t lineNo = 0;
    while (std::getline(file, line))
    {
        ++lineNo;
        std::string code = line.substr(0, line.find('#'));
        if (code.find_first_not_of(" \t\r") == std::string::npos)
        {
            continue;
        }
        try
        {
            trace.push_back(parse_trace_line(code));
        }
        catch (const std::exception& e)
        {
            throw std::invalid_argument(path + ":" + std::to_string(lineNo) + ": " + e.what());
        }
    }
    return trace;
}
