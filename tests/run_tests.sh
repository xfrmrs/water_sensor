#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
TEST_DIR="$ROOT_DIR/tests"
BIN_DIR="$TEST_DIR/.bin"

mkdir -p "$BIN_DIR"

echo "Compiling test_json_helpers.cpp..."
g++ -std=c++17 -Wall -Wextra -Wno-unused-variable -I"$TEST_DIR" "$TEST_DIR/test_json_helpers.cpp" -o "$BIN_DIR/test_json_helpers"

echo "Running test_json_helpers..."
"$BIN_DIR/test_json_helpers"

echo "Compiling test_config.cpp..."
g++ -std=c++17 -Wall -Wextra -Wno-unused-variable -I"$TEST_DIR" "$TEST_DIR/test_config.cpp" -o "$BIN_DIR/test_config"

echo "Running test_config..."
"$BIN_DIR/test_config"

echo "All host tests passed."
