#!/bin/bash
# -----------------------------------------------------------------------
# Code Coverage Generation Script
#
# This script runs tests with coverage instrumentation and generates
# code coverage reports using lcov. It should be used as the primary
# coverage generation tool for CI/CD pipelines.
# -----------------------------------------------------------------------

set -e

# Default output directory - matching what's expected in CI/CD workflow
OUTPUT_DIR="coverage"
HTML_REPORT=false

# Parse arguments
while [[ "$#" -gt 0 ]]; do
  case $1 in
    --output=*) OUTPUT_DIR="${1#*=}"; shift ;;
    --html) HTML_REPORT=true; shift ;;
    *) echo "Unknown parameter: $1"; exit 1 ;;
  esac
done

echo "====================================================="
echo "RUNNING CODE COVERAGE GENERATION"
echo "====================================================="
echo "Coverage data will be stored in: $OUTPUT_DIR"

# Ensure build directory exists
mkdir -p build

# Configure CMake with coverage flags
cd build
echo "Configuring project with coverage enabled..."
cmake .. -DCODE_COVERAGE=ON -DBUILD_TESTS=ON
echo "Building project with coverage instrumentation..."
make -j$(nproc)

# Run tests sequentially to avoid coverage data corruption from parallel execution
echo "====================================================="
echo "Running tests with coverage instrumentation..."
echo "====================================================="
cd bin
for test_bin in *_test; do
    if [ -x "$test_bin" ]; then
        echo "Running $test_bin..."
        ./$test_bin
        # Small delay to ensure coverage data is properly written
        sleep 0.1
    fi
done
cd ..

# Create output directory - ensure absolute path
mkdir -p "../$OUTPUT_DIR"

# Generate coverage report
echo "====================================================="
echo "Generating coverage reports..."
echo "====================================================="
echo "Capturing raw coverage data..."
lcov --capture --directory . --output-file "../$OUTPUT_DIR/coverage.info"

# Remove unwanted directories from coverage report
echo "Filtering coverage data to include only relevant source files..."
lcov --remove "../$OUTPUT_DIR/coverage.info" \
    '/usr/*' \
    '*/include/gtest/*' \
    '*/gtest/*' \
    '*/test/*' \
    '*/tests/*' \
    '*_test.cpp' \
    '*_test.cc' \
    '*Mock*.hpp' \
    --output-file "../$OUTPUT_DIR/coverage_filtered.info"

# Keep only the directories we want to cover
echo "Extracting coverage data for project source directories..."
lcov --extract "../$OUTPUT_DIR/coverage_filtered.info" \
    '*/Controller/src/*' \
    '*/Controller/inc/*' \
    '*/Middleware/src/*' \
    '*/Middleware/inc/*' \
    '*/zmq/src/*' \
    '*/zmq/inc/*' \
    '*/modules/cluster-display/ClusterDisplay/src/*' \
    '*/modules/cluster-display/ClusterDisplay/inc/*' \
    --output-file "../$OUTPUT_DIR/coverage.info"

# Clean up temporary file
rm -f "../$OUTPUT_DIR/coverage_filtered.info"

echo "====================================================="
echo "Coverage Summary:"
lcov --list "../$OUTPUT_DIR/coverage.info"
echo "====================================================="

# Generate HTML report if requested
if [ "$HTML_REPORT" = true ]; then
    echo "Generating HTML report..."
    genhtml "../$OUTPUT_DIR/coverage.info" --output-directory "../$OUTPUT_DIR/html"
    echo "HTML report generated in $OUTPUT_DIR/html/index.html"
fi

echo "Coverage information saved to $OUTPUT_DIR/coverage.info"
echo "Coverage analysis completed successfully!"

# Return to project root
cd ..
