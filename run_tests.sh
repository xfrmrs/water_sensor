#!/bin/bash
set -e
echo "Compiling test_json_helpers.cpp..."
g++ -std=c++11 -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -I./mocks test_json_helpers.cpp -o test_json_helpers
echo "Running test_json_helpers..."
./test_json_helpers

echo "Compiling test_config.cpp..."
g++ -I. -I./test_mocks test_config.cpp -o run_tests -DARDUINO

echo "Running test_config..."
./run_tests

echo "Cleaning up..."
rm run_tests test_json_helpers
