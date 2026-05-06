#!/bin/bash
set -e

echo "Compiling tests..."
g++ -I. -I./test_mocks test_config.cpp -o run_tests

echo "Running tests..."
./run_tests

echo "Cleaning up..."
rm run_tests
