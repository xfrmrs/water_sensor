#!/bin/bash
set -e

echo "Compiling tests..."
g++ -I. -I./tests/mocks tests/test_config.cpp -o tests/test_runner

echo "Running tests..."
./tests/test_runner

echo "Test run complete."
