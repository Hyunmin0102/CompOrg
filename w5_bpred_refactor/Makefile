CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -O0
# -MMD: .o를 만들 때 그 .cpp가 include한 헤더 목록을 .d 파일로 같이 쓴다
# -MP : 헤더를 지워도 "No rule to make target xxx.h" 에러가 나지 않게 한다
DEPFLAGS := -MMD -MP

# 이 폴더의 .cpp를 모두 빌드한다 → 새 예측기 .cpp를 추가해도 Makefile은 고치지 않는다.
# 링크 에러 실험용: make EXCLUDE=gshare.cpp 처럼 빼고 싶은 .cpp를 지정한다 (README 참고)
EXCLUDE :=
SRCS    := $(filter-out $(EXCLUDE),$(wildcard *.cpp))
OBJS    := $(SRCS:.cpp=.o)
DEPS    := $(SRCS:.cpp=.d)

# trace는 W4 폴더의 것을 그대로 쓴다. W4와 출력(경로 표기 포함)을 맞추려고 W4 폴더 안에서 실행한다.
W4_DIR := ../w4_bpred
TRACES := tests/loop_n4.txt tests/loop_n9.txt tests/alternate.txt \
          tests/always_taken.txt tests/always_not_taken.txt \
          tests/correlated.txt tests/mixed.txt

# 회귀 테스트용 이전 주차 바이너리. 이전 주차 폴더에는 아무것도 만들지 않도록 이 폴더(build/)에 빌드한다.
W1_DIR  := ../w1_decoder
W2_DIR  := ../w2_simulator
W3_DIR  := ../w3_pipeline
W2_SRCS := $(W2_DIR)/main.cpp $(W2_DIR)/cpu.cpp $(W2_DIR)/decoder.cpp
W2_HDRS := $(W2_DIR)/cpu.h $(W2_DIR)/decoder.h
W3_SRCS := $(W3_DIR)/main.cpp $(W3_DIR)/cpu.cpp $(W3_DIR)/decoder.cpp $(W3_DIR)/timing.cpp
W3_HDRS := $(W3_DIR)/cpu.h $(W3_DIR)/decoder.h $(W3_DIR)/timing.h

all: bpred

# 링크: .o를 모두 묶어 실행 파일을 만든다. .o 중 하나라도 바뀌면 다시 링크한다
bpred: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# 컴파일: .cpp 하나 → .o 하나. 바뀐 .cpp(또는 그 .cpp가 include한 헤더)만 다시 컴파일한다
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# 앞에서 만든 .d 파일(헤더 의존성)을 읽는다. 처음 빌드할 때는 없으므로 -include (없어도 에러 아님)
-include $(DEPS)

# make run: W4의 모든 테스트 trace로 정확도 비교 표
run: bpred
	cd $(W4_DIR) && ../w5_bpred_refactor/bpred $(TRACES)

# make top: 비교 표 + 예측기별 오예측 Top 10
top: bpred
	cd $(W4_DIR) && ../w5_bpred_refactor/bpred --top $(TRACES)

# make compare: W5 출력이 W4 bpred 출력과 trace별로 완전히 같은지 diff로 확인
compare: bpred
	./compare_w4.sh

build/w1_decoder: $(W1_DIR)/decoder.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@

build/w2_sim: $(W2_SRCS) $(W2_HDRS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(W2_SRCS) -o $@

build/w3_sim: $(W3_SRCS) $(W3_HDRS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(W3_SRCS) -o $@

build/w4_bpred: $(W4_DIR)/bpred.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@

# make regress: 이전 주차가 여전히 빌드·통과하는지 확인한다. 하나라도 실패하면 멈춘다.
#   W2·W3 바이너리는 programs/ 를 상대 경로로 읽으므로 각 주차 폴더 안에서 실행한다.
#   W3 sim은 시작할 때 run_tests()(W1 assert 복사본 + W2 테스트 + W3 통계 assert)를 돌린다.
#   W4는 이 폴더에서 다시 빌드해 make run과 같은 표를 출력한다.
regress: build/w1_decoder build/w2_sim build/w3_sim build/w4_bpred
	@echo "== W1 decoder regression =="
	@./build/w1_decoder < /dev/null
	@echo "== W2 simulator regression =="
	@cd $(W2_DIR) && ../w5_bpred_refactor/build/w2_sim programs/sum1to10.txt > /dev/null && echo "all tests passed"
	@echo "== W3 pipeline regression =="
	@cd $(W3_DIR) && ../w5_bpred_refactor/build/w3_sim programs/sum1to10.txt > ../w5_bpred_refactor/build/out.txt \
		&& sed -n '1p;/=== pipeline ===/,$$p' ../w5_bpred_refactor/build/out.txt
	@echo "== W4 branch predictor regression =="
	@cd $(W4_DIR) && ../w5_bpred_refactor/build/w4_bpred $(TRACES)

clean:
	rm -f bpred *.o *.d
	rm -rf build

.PHONY: all run top compare regress clean
