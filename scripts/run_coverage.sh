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
# Use CMAKE_FLAGS if provided, otherwise use defaults
if [ -n "$CMAKE_FLAGS" ]; then
    echo "Using CMAKE_FLAGS: $CMAKE_FLAGS"
    cmake .. $CMAKE_FLAGS
else
    echo "Using default coverage flags..."
    cmake .. -DCODE_COVERAGE=ON -DBUILD_TESTS=ON
fi
echo "Building project with coverage instrumentation..."
make -j$(nproc)

# Run tests sequentially to avoid coverage data corruption from parallel execution
echo "====================================================="
echo "Running tests with coverage instrumentation..."
echo "====================================================="
cd bin

# Check if any test binaries exist
test_count=0
for test_bin in *_test; do
    if [ -x "$test_bin" ]; then
        test_count=$((test_count + 1))
    fi
done

if [ $test_count -eq 0 ]; then
    echo "No test binaries found in bin/ directory"
    echo "This might be because BUILD_TESTS=OFF or tests failed to compile"
    cd ..
    exit 1
fi

echo "Found $test_count test binary(ies)"
for test_bin in *_test; do
    if [ -x "$test_bin" ]; then
        echo "Running $test_bin..."
        ./$test_bin
        # Longer delay to ensure coverage data is properly written and avoid race conditions
        sleep 0.5
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

# Check lcov version to determine which ignore-errors arguments to use
LCOV_VERSION=$(lcov --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+' | head -n1)
LCOV_MAJOR=$(echo $LCOV_VERSION | cut -d. -f1)
LCOV_MINOR=$(echo $LCOV_VERSION | cut -d. -f2)

# Use mismatch argument for newer lcov versions (>= 1.14)
if [ "$LCOV_MAJOR" -gt 1 ] || ([ "$LCOV_MAJOR" -eq 1 ] && [ "$LCOV_MINOR" -ge 14 ]); then
    IGNORE_ERRORS="mismatch,negative"
else
    IGNORE_ERRORS="negative"
fi

echo "Using lcov version $LCOV_VERSION with ignore-errors: $IGNORE_ERRORS"

lcov --capture --directory . --output-file "../$OUTPUT_DIR/coverage.info" \
    --ignore-errors $IGNORE_ERRORS \
    --rc geninfo_unexecuted_blocks=1 || {
    echo "ERROR: Failed to capture coverage data"
    echo "This might be because:"
    echo "1. No tests were run"
    echo "2. Coverage instrumentation was not enabled"
    echo "3. No source files were compiled with coverage flags"
    cd ..
    exit 1
}

# Remove unwanted directories from coverage report
echo "Filtering coverage data to include only relevant source files..."
lcov --remove "../$OUTPUT_DIR/coverage.info" \
    '/usr/*' \
    '*/include/gtest/*' \
    '*/test/*' \
    '*/tests/*' \
    '*_test.cpp' \
    '*_test.cc' \
    '*Mock*.hpp' \
    '*.hpp' \
    '*/inc/*' \
    --output-file "../$OUTPUT_DIR/coverage_filtered.info" \
    --ignore-errors unused

# Keep only the directories we want to cover
echo "Extracting coverage data for project source directories..."
lcov --extract "../$OUTPUT_DIR/coverage_filtered.info" \
    '*/Middleware/src/*' \
    --output-file "../$OUTPUT_DIR/coverage_temp.info" \
    --ignore-errors unused

# Apply LCOV exclusions from source code comments
echo "Applying LCOV exclusions from source code..."
lcov --remove "../$OUTPUT_DIR/coverage_temp.info" \
    '*/Middleware/src/main.cpp:46-181' \
    --output-file "../$OUTPUT_DIR/coverage.info" \
    --ignore-errors unused

# Clean up temporary file
rm -f "../$OUTPUT_DIR/coverage_temp.info"

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
