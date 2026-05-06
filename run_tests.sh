#!/bin/bash
set -e

echo "Compiling tests..."
g++ -I. -I./tests/mocks tests/test_config.cpp -o tests/test_runner

echo "Running tests..."
./tests/test_runner

echo "Test run complete."
echo "Compiling test_json_helpers.cpp..."
g++ -std=c++11 -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -I./mocks test_json_helpers.cpp -o test_json_helpers
echo "Running tests..."
./test_json_helpers

echo "Compiling tests..."
g++ -I. -I./test_mocks test_config.cpp -o run_tests

echo "Running tests..."
./run_tests

echo "Cleaning up..."
rm run_tests
