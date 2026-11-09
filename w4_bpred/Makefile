CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g

# 테스트 trace: tests/<이름>.txt. 규칙은 아래 traces 타깃에 있다.
TRACES := tests/loop_n4.txt tests/loop_n9.txt tests/alternate.txt \
          tests/always_taken.txt tests/always_not_taken.txt \
          tests/correlated.txt tests/mixed.txt

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

clean:
	rm -f gen_trace

.PHONY: traces clean
