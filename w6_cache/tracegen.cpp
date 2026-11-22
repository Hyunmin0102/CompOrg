// W6: 행렬 순회 trace 생성기
//   rows x cols 행렬(원소 elemBytes 바이트)이 메모리에 행 우선(row-major)으로 놓여 있을 때,
//   행 우선 / 열 우선으로 모든 원소를 한 번씩 접근하는 trace를 표준 출력에 쓴다.
//     원소 (i, j)의 주소 = base + (i * cols + j) * elemBytes
//
//   사용법: ./tracegen <row|col> <rows> <cols> <elemBytes> [r|w|rw] [base]
//     row|col   : 순회 순서. row = for i { for j }, col = for j { for i }
//     rows cols : 행렬 크기
//     elemBytes : 원소 크기 (예: int = 4, double = 8)
//     r|w|rw    : 원소마다 읽기만 / 쓰기만 / 읽고 쓰기(A[i][j] += 1 같은 갱신). 기본 r
//     base      : 행렬 시작 주소. 0x 접두사로 16진수 가능. 기본 0x10000
//
//   예: ./tracegen col 128 128 4 rw > tests/col_128x128_rw.txt
//
//   이 프로그램은 위치 인자(positional argument)만 받는다.
//   --key=value 형식 파싱은 main.cpp TODO(7)에서 직접 구현한다.
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

struct GenOptions {
    bool        colMajor  = false;
    uint64_t    rows      = 0;
    uint64_t    cols      = 0;
    uint64_t    elemBytes = 0;
    bool        doRead    = true;
    bool        doWrite   = false;
    uint64_t    base      = 0x10000;
    std::string modeName  = "r";
};

// 부호 없는 정수. base = 0이면 "0x..."는 16진수, 나머지는 10진수로 읽는다(std::stoull의 base 0).
//   "12abc", "-1", ""은 오류
static uint64_t parse_number(const std::string& s, const std::string& what, int base) {
    if (s.empty() || s[0] == '-')
    {
        throw std::invalid_argument(what + " must be a non-negative integer, got '" + s + "'");
    }
    size_t used = 0;
    uint64_t v = 0;
    try
    {
        v = std::stoull(s, &used, base);
    }
    catch (const std::logic_error&)
    {
        throw std::invalid_argument(what + " must be a non-negative integer, got '" + s + "'");
    }
    if (used != s.size())
    {
        throw std::invalid_argument(what + " must be a non-negative integer, got '" + s + "'");
    }
    return v;
}

static GenOptions parse_gen_args(int argc, char* argv[]) {
    if (argc < 5 || argc > 7)
    {
        throw std::invalid_argument("wrong number of arguments");
    }

    GenOptions opt;
    const std::string order = argv[1];
    if (order == "row")
    {
        opt.colMajor = false;
    }
    else if (order == "col")
    {
        opt.colMajor = true;
    }
    else
    {
        throw std::invalid_argument("order must be row or col, got '" + order + "'");
    }

    opt.rows      = parse_number(argv[2], "rows", 10);
    opt.cols      = parse_number(argv[3], "cols", 10);
    opt.elemBytes = parse_number(argv[4], "elemBytes", 10);
    if (opt.rows == 0 || opt.cols == 0 || opt.elemBytes == 0)
    {
        throw std::invalid_argument("rows, cols, elemBytes must be at least 1");
    }

    if (argc >= 6)
    {
        opt.modeName = argv[5];
        if (opt.modeName == "r")
        {
            opt.doRead = true;
            opt.doWrite = false;
        }
        else if (opt.modeName == "w")
        {
            opt.doRead = false;
            opt.doWrite = true;
        }
        else if (opt.modeName == "rw")
        {
            opt.doRead = true;
            opt.doWrite = true;
        }
        else
        {
            throw std::invalid_argument("mode must be r, w or rw, got '" + opt.modeName + "'");
        }
    }
    if (argc >= 7)
    {
        opt.base = parse_number(argv[6], "base", 0);
    }
    return opt;
}

static void emit(std::ostream& os, const GenOptions& opt, uint64_t i, uint64_t j) {
    const uint64_t addr = opt.base + (i * opt.cols + j) * opt.elemBytes;
    if (opt.doRead)
    {
        os << "R 0x" << std::hex << addr << std::dec << "\n";
    }
    if (opt.doWrite)
    {
        os << "W 0x" << std::hex << addr << std::dec << "\n";
    }
}

static void print_usage(std::ostream& os, const char* prog) {
    os << "usage: " << prog << " <row|col> <rows> <cols> <elemBytes> [r|w|rw] [base]\n";
}

int main(int argc, char* argv[]) {
    GenOptions opt;
    try
    {
        opt = parse_gen_args(argc, argv);
    }
    catch (const std::invalid_argument& e)
    {
        std::cerr << "error: " << e.what() << "\n";
        print_usage(std::cerr, argv[0]);
        return 1;
    }

    std::ostream& os = std::cout;
    os << "# tracegen: " << (opt.colMajor ? "col" : "row") << "-major traversal of "
       << opt.rows << " x " << opt.cols << " matrix, " << opt.elemBytes << "-byte elements, mode "
       << opt.modeName << ", base 0x" << std::hex << opt.base << std::dec << "\n";
    os << "# matrix bytes = " << opt.rows * opt.cols * opt.elemBytes
       << ", row stride = " << opt.cols * opt.elemBytes << " bytes\n";

    if (opt.colMajor)
    {
        for (uint64_t j = 0; j < opt.cols; ++j)
        {
            for (uint64_t i = 0; i < opt.rows; ++i)
            {
                emit(os, opt, i, j);
            }
        }
    }
    else
    {
        for (uint64_t i = 0; i < opt.rows; ++i)
        {
            for (uint64_t j = 0; j < opt.cols; ++j)
            {
                emit(os, opt, i, j);
            }
        }
    }
    return 0;
}
