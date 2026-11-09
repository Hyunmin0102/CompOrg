CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g

# 테스트 trace: tests/<이름>.txt. 규칙은 아래 traces 타깃에 있다.
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

all: bpred gen_trace

bpred: bpred.cpp
	$(CXX) $(CXXFLAGS) bpred.cpp -o bpred

gen_trace: gen_trace.cpp
	$(CXX) $(CXXFLAGS) gen_trace.cpp -o gen_trace

# make traces: tests/ 의 trace를 다시 만든다 (커밋된 파일과 같아야 한다)
traces: gen_trace
	./gen_trace loop             50   tests/loop_n4.txt 4
	./gen_trace loop             100  tests/loop_n9.txt 9
	./gen_trace alternate        20   tests/alternate.txt
	./gen_trace always_taken     20   tests/always_taken.txt
	./gen_trace always_not_taken 20   tests/always_not_taken.txt
	./gen_trace correlated       2000 tests/correlated.txt
	./gen_trace mixed            4000 tests/mixed.txt 3

# make run: 모든 테스트 trace에 대해 정확도 비교 표 출력
run: bpred
	./bpred $(TRACES)

# make top: 비교 표 + 예측기별 오예측 Top 10
top: bpred
	./bpred --top $(TRACES)

build/w1_decoder: $(W1_DIR)/decoder.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@

build/w2_sim: $(W2_SRCS) $(W2_HDRS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(W2_SRCS) -o $@

build/w3_sim: $(W3_SRCS) $(W3_HDRS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(W3_SRCS) -o $@

# make regress: 이전 주차가 여전히 빌드·통과하는지 확인한다. 하나라도 실패하면 멈춘다.
#   W2·W3 바이너리는 programs/ 를 상대 경로로 읽으므로 각 주차 폴더 안에서 실행한다.
#   W3 sim은 시작할 때 run_tests()(W1 assert 복사본 + W2 테스트 + W3 통계 assert)를 돌린다.
regress: build/w1_decoder build/w2_sim build/w3_sim
	@echo "== W1 decoder regression =="
	@./build/w1_decoder < /dev/null
	@echo "== W2 simulator regression =="
	@cd $(W2_DIR) && ../w4_bpred/build/w2_sim programs/sum1to10.txt > /dev/null && echo "all tests passed"
	@echo "== W3 pipeline regression =="
	@cd $(W3_DIR) && ../w4_bpred/build/w3_sim programs/sum1to10.txt > ../w4_bpred/build/out.txt \
		&& sed -n '1p;/=== pipeline ===/,$$p' ../w4_bpred/build/out.txt

clean:
	rm -f bpred gen_trace
	rm -rf build

.PHONY: all traces run top regress clean
