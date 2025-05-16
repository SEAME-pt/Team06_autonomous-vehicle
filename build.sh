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

# Run CMake with options
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
