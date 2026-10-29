CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g

SRCS := main.cpp cpu.cpp decoder.cpp timing.cpp
HDRS := cpu.h decoder.h timing.h

EXAMPLES := programs/loaduse.txt programs/dist2.txt programs/nodep.txt programs/sum1to10.txt

# 회귀 테스트용 이전 주차 바이너리. 이전 주차 폴더에는 아무것도 만들지 않도록 이 폴더(build/)에 빌드한다.
W1_DIR := ../w1_decoder
W2_DIR := ../w2_simulator
W2_SRCS := $(W2_DIR)/main.cpp $(W2_DIR)/cpu.cpp $(W2_DIR)/decoder.cpp
W2_HDRS := $(W2_DIR)/cpu.h $(W2_DIR)/decoder.h

sim: $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o sim

build/w1_decoder: $(W1_DIR)/decoder.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@

build/w2_sim: $(W2_SRCS) $(W2_HDRS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(W2_SRCS) -o $@

run: sim
	./sim programs/sum1to10.txt

# make test: W1 → W2 → W3 순서로 돌리고, 하나라도 실패(assert, 0이 아닌 종료 코드)하면 멈춘다.
#   W1: stdin이 비어 있으면 assert만 돌고 끝난다.
#   W2: programs/sum1to10.txt를 상대 경로로 읽으므로 w2_simulator/ 안에서 실행한다.
#   W3: sim이 시작할 때 run_tests()(W1 assert 복사본 + W2 테스트 + W3 통계)를 돌린 뒤
#       예제마다 forwarding on/off 통계를 출력한다. trace·레지스터 덤프는 생략하고 통계만 보여 준다.
test: build/w1_decoder build/w2_sim sim
	@echo "== W1 decoder regression =="
	@./build/w1_decoder < /dev/null
	@echo "== W2 simulator regression =="
	@cd $(W2_DIR) && ../w3_pipeline/build/w2_sim programs/sum1to10.txt > /dev/null && echo "all tests passed"
	@echo "== W3 pipeline =="
	@# 파이프(|)로 sed에 넘기면 종료 코드가 sed 것이 되어 sim 실패가 묻힌다 → 파일에 저장한 뒤 거른다
	@for p in $(EXAMPLES); do \
		echo "--- $$p (forwarding) ---"; \
		./sim $$p > build/out.txt || exit 1; sed -n '/=== pipeline ===/,$$p' build/out.txt; \
		echo "--- $$p (no forwarding) ---"; \
		./sim $$p --no-forwarding > build/out.txt || exit 1; sed -n '/=== pipeline ===/,$$p' build/out.txt; \
	done

clean:
	rm -f sim
	rm -rf build

.PHONY: run test clean
