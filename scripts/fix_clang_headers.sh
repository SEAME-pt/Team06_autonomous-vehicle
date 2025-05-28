#!/bin/bash
# Script to fix clang-tidy include path issues on CI

set -e

echo "Fixing clang include paths for CI environment..."

# Create directories for clang includes
mkdir -p /usr/lib/clang/6.0/include
mkdir -p /usr/lib/clang/10.0.0/include

# Copy standard header files to clang include directories
if [ -f "/usr/include/stddef.h" ]; then
  echo "Copying stddef.h to clang include directories"
  cp -f /usr/include/stddef.h /usr/lib/clang/6.0/include/
  cp -f /usr/include/stddef.h /usr/lib/clang/10.0.0/include/
else
  echo "Warning: stddef.h not found in /usr/include"
fi

if [ -f "/usr/include/stdarg.h" ]; then
  echo "Copying stdarg.h to clang include directories"
  cp -f /usr/include/stdarg.h /usr/lib/clang/6.0/include/
  cp -f /usr/include/stdarg.h /usr/lib/clang/10.0.0/include/
fi

if [ -f "/usr/include/stdbool.h" ]; then
  echo "Copying stdbool.h to clang include directories"
  cp -f /usr/include/stdbool.h /usr/lib/clang/6.0/include/
  cp -f /usr/include/stdbool.h /usr/lib/clang/10.0.0/include/
fi

# Find all GCC include paths
echo "Finding GCC include paths..."
GCC_INCLUDE_PATHS=$(gcc -xc++ -E -v - < /dev/null 2>&1 | grep "^ " | tr "\n" ":")

# Export GCC include paths for clang-tidy to use
export CPLUS_INCLUDE_PATH="$GCC_INCLUDE_PATHS"
echo "Exported CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH"

echo "Clang header fixes completed."
