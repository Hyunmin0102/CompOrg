CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g

decoder: decoder.cpp
	$(CXX) $(CXXFLAGS) decoder.cpp -o decoder

run: decoder
	./decoder

clean:
	rm -f decoder

.PHONY: run clean