#!/bin/bash
set -e

echo "Compiling tests..."
g++ -std=c++11 -I tests/mocks -I . tests/test_json_helpers.cpp -o tests/test_runner

echo "Running tests..."
./tests/test_runner
