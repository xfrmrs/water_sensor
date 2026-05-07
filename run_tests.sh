#!/bin/bash
set -e
echo "Compiling test_json_helpers.cpp..."
g++ -std=c++11 -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -I./mocks test_json_helpers.cpp -o test_json_helpers
echo "Running tests..."
./test_json_helpers

echo "Compiling tests..."
g++ -DARDUINO -I. -I./test_mocks ip_utils.cpp test_config.cpp -o run_tests

echo "Running tests..."
./run_tests

echo "Cleaning up..."
rm run_tests
