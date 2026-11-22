CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -O0
# -MMD: .o를 만들 때 그 .cpp가 include한 헤더 목록을 .d 파일로 같이 쓴다
# -MP : 헤더를 지워도 "No rule to make target xxx.h" 에러가 나지 않게 한다
DEPFLAGS := -MMD -MP

# W5와 달리 main()이 있는 .cpp가 셋(main, tests, tracegen)이라 wildcard로 전부 묶을 수 없다.
# 실행 파일끼리 공유하는 .cpp는 COMMON_SRCS에 둔다. W7에서 새 .cpp(예: 계층)를 추가하면 여기에 넣는다.
COMMON_SRCS := cache.cpp trace.cpp
COMMON_OBJS := $(COMMON_SRCS:.cpp=.o)
SIM_OBJS    := main.o $(COMMON_OBJS)
TEST_OBJS   := tests.o $(COMMON_OBJS)
GEN_OBJS    := tracegen.o
DEPS        := $(wildcard *.d)

# make traces로 만드는 행렬 순회 trace. 128 x 128 x 4B = 64KB (기본 캐시 32KB보다 크다)
GEN_TRACES := tests/row_128x128.txt tests/col_128x128.txt \
              tests/row_128x128_rw.txt tests/col_128x128_rw.txt

all: sim test tracegen

# 링크: .o를 묶어 실행 파일을 만든다
sim: $(SIM_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

test: $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

tracegen: $(GEN_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# 컴파일: .cpp 하나 → .o 하나. 바뀐 .cpp(또는 그 .cpp가 include한 헤더)만 다시 컴파일한다
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(DEPS)

# make check: 모든 단계 테스트 실행 (./test all). 단계 하나만은 ./test v1 처럼 직접 실행한다
check: test
	./test all

# make traces: tracegen으로 tests/의 행렬 순회 trace를 다시 만든다
traces: $(GEN_TRACES)

tests/row_128x128.txt: tracegen
	./tracegen row 128 128 4 r > $@
tests/col_128x128.txt: tracegen
	./tracegen col 128 128 4 r > $@
tests/row_128x128_rw.txt: tracegen
	./tracegen row 128 128 4 rw > $@
tests/col_128x128_rw.txt: tracegen
	./tracegen col 128 128 4 rw > $@

clean:
	rm -f sim test tracegen *.o *.d

.PHONY: all check traces clean
