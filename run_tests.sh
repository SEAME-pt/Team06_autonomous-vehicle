#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# Install required dependencies
apt-get update -qq && apt-get install -y lcov gcovr -qq || true

# Create and navigate to build directory
mkdir -p build
cd build

# Configure CMake with coverage enabled
cmake .. -DCODE_COVERAGE=ON

# Build the project
make

# Track test status
TEST_FAILURES=0
TEST_TOTAL=0
FAILED_TESTS=""

cd bin

# Create directory for test reports
mkdir -p reports

# Function to run a test and handle failures gracefully
run_test() {
    local test_name=$1
    local timestamp=$(date +"%T")

    TEST_TOTAL=$((TEST_TOTAL + 1))
    echo "[$timestamp] Running $test_name..."

    # Create report directory for this test
    mkdir -p "reports/$test_name"

    # Run the test with gtest output formatter for XML reports
    if ./$test_name --gtest_output=xml:"reports/$test_name/results.xml"; then
        echo "[$timestamp] ✅ PASS: $test_name"
    else
        TEST_FAILURES=$((TEST_FAILURES + 1))
        FAILED_TESTS="$FAILED_TESTS\n  - $test_name"
        echo "[$timestamp] ❌ FAIL: $test_name"
    fi
    echo ""
}

# Run all test executables
for test_executable in *_test; do
    if [ -x "$test_executable" ]; then
        run_test "$test_executable"
    fi
done

cd ..

# Generate coverage report regardless of test results
echo "Generating coverage report..."
make coverage || true
echo "Coverage report generated in build/coverage/html"

# Print test summary
echo "-------------------------------------------"
echo "Test Summary: $((TEST_TOTAL - TEST_FAILURES))/$TEST_TOTAL tests passed"

if [ $TEST_FAILURES -gt 0 ]; then
    echo -e "Failed tests:$FAILED_TESTS"
    exit 1
else
    echo "All tests passed successfully!"
    exit 0
fi
