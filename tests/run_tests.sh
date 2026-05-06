#!/bin/bash
set -e

echo "Compiling tests..."
g++ tests/test_json_helpers.cpp tests/mocks/Arduino_JSON.cpp -o tests/test_runner -I tests/mocks -I tests

echo "Running tests..."
tests/test_runner

echo "Tests passed successfully."
g++ -std=c++11 -I tests/mocks -I . tests/test_json_helpers.cpp -o tests/test_runner

echo "Running tests..."
./tests/test_runner
