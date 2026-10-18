CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g

SRCS := main.cpp cpu.cpp decoder.cpp
HDRS := cpu.h decoder.h

sim: $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o sim

run: sim
	./sim programs/sum1to10.txt

clean:
	rm -f sim

.PHONY: run clean
