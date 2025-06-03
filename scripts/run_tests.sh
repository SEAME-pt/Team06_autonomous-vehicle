#!/bin/bash
# -----------------------------------------------------------------------
# Test Runner Script
#
# This script builds and runs all unit tests for the project.
# It collects test results and returns a non-zero exit code if any tests fail.
# -----------------------------------------------------------------------

# Exit immediately if a command exits with a non-zero status.
set -e

# Get the project root directory (parent of scripts directory)
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
echo "Project root: $PROJECT_ROOT"

# Check if we need to install dependencies
if [ ! -f "/usr/include/zmq.h" ]; then
    echo "Installing ZeroMQ dependencies..."
    if [ $(id -u) -eq 0 ]; then
        apt-get update
        apt-get install -y libzmq3-dev
    else
        sudo apt-get update
        sudo apt-get install -y libzmq3-dev
    fi
fi

# Create and navigate to build directory
mkdir -p "$PROJECT_ROOT/build"
cd "$PROJECT_ROOT/build"

# Configure CMake with coverage enabled
echo "Configuring project with tests enabled..."
cmake "$PROJECT_ROOT" -DCODE_COVERAGE=ON -DBUILD_TESTS=ON

# Build the project
echo "Building project and tests..."
make

# Track test status
TEST_FAILURES=0
TEST_TOTAL=0
FAILED_TESTS=""

cd bin

# Create directory for test reports
mkdir -p reports

echo "====================================================="
echo "RUNNING UNIT TESTS"
echo "====================================================="

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

# Print test summary
echo "====================================================="
echo "TEST SUMMARY"
echo "====================================================="
echo "Results: $((TEST_TOTAL - TEST_FAILURES))/$TEST_TOTAL tests passed"

if [ $TEST_FAILURES -gt 0 ]; then
    echo -e "Failed tests:$FAILED_TESTS"
    exit 1
else
    echo "All tests passed successfully!"
    exit 0
fi
