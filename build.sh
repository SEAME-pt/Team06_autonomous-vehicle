#!/bin/bash

# Parse command line arguments
ENABLE_COVERAGE=0

for arg in "$@"
do
    case $arg in
        --coverage)
        ENABLE_COVERAGE=1
        shift
        ;;
    esac
done

# Create build directory if it doesn't exist
mkdir -p build
cd build

# CMake options
CMAKE_OPTIONS="-Wno-dev"

# Add coverage if requested
if [ $ENABLE_COVERAGE -eq 1 ]; then
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DCODE_COVERAGE=ON"
    echo "Enabling code coverage"
fi

# Add any additional CMake flags from environment
if [ ! -z "$CMAKE_FLAGS" ]; then
    # Split CMAKE_FLAGS into array and add each flag
    IFS=' ' read -ra FLAGS <<< "$CMAKE_FLAGS"
    for flag in "${FLAGS[@]}"; do
        CMAKE_OPTIONS="$CMAKE_OPTIONS $flag"
    done
    echo "Adding CMake flags: $CMAKE_FLAGS"
fi

# Run CMake with options
echo "Running CMake with options: $CMAKE_OPTIONS"
cmake $CMAKE_OPTIONS ..

# Build the project
make

# Run coverage if requested
if [ $ENABLE_COVERAGE -eq 1 ]; then
    echo "Generating coverage report..."
    make coverage
    echo "Coverage report generated in build/coverage/html"
    echo "Open build/coverage/html/index.html in your browser to view the report"
else
    echo "Build completed. Run ./bin/Middleware to start the application."
fi
