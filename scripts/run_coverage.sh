#!/bin/bash
set -e

# Default output directory
OUTPUT_DIR="build/coverage"
HTML_REPORT=false

# Parse arguments
while [[ "$#" -gt 0 ]]; do
  case $1 in
    --output=*) OUTPUT_DIR="${1#*=}"; shift ;;
    --html) HTML_REPORT=true; shift ;;
    *) echo "Unknown parameter: $1"; exit 1 ;;
  esac
done

echo "Running code coverage..."

# Ensure build directory exists
mkdir -p build

# Configure CMake with coverage flags
cd build
cmake .. -DCODE_COVERAGE=ON -DBUILD_TESTS=ON
make -j$(nproc)

# Run tests
echo "Running tests with coverage instrumentation..."
cd bin
for test_bin in *_test; do
    if [ -x "$test_bin" ]; then
        echo "Running $test_bin..."
        ./$test_bin
    fi
done
cd ..

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Generate coverage report
echo "Generating coverage reports..."
lcov --capture --directory . --output-file "$OUTPUT_DIR/coverage.info"
lcov --remove "$OUTPUT_DIR/coverage.info" '/usr/*' --output-file "$OUTPUT_DIR/coverage.info"
lcov --list "$OUTPUT_DIR/coverage.info"

# Generate HTML report if requested
if [ "$HTML_REPORT" = true ]; then
    echo "Generating HTML report..."
    genhtml "$OUTPUT_DIR/coverage.info" --output-directory "$OUTPUT_DIR/html"
    echo "HTML report generated in $OUTPUT_DIR/html/index.html"
fi

echo "Coverage information saved to $OUTPUT_DIR/coverage.info"
echo "Coverage analysis completed successfully!"

# Return to project root
cd ..
