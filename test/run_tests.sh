#!/bin/bash
set -e
echo "Extracting actual functions from config.ino..."
python3 extract_funcs.py
echo "Compiling tests..."
g++ test_config.cpp -o test_config -Wall -Wextra
echo "Running tests..."
./test_config
echo "Success."
