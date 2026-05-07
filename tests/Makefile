CXX = g++
CXXFLAGS = -std=c++11 -Wall -I. -I..

TEST_BINS = runner

all: $(TEST_BINS)

runner: test_config.cpp mock_arduino.h mock_common.h common.h ../config.ino
	$(CXX) $(CXXFLAGS) -o $@ test_config.cpp

test: runner
	./runner

clean:
	rm -f $(TEST_BINS)

.PHONY: all test clean
