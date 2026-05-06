#!/bin/bash
set -e
echo "Compiling test_json_helpers.cpp..."
g++ -std=c++11 -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -I./mocks test_json_helpers.cpp -o test_json_helpers
echo "Running tests..."
./test_json_helpers
