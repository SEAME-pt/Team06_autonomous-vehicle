#!/bin/bash
set -e

# Default behavior: check mode
MODE="check"
FIX_FLAG="--dry-run --Werror"

# Parse arguments
while [[ "$#" -gt 0 ]]; do
  case $1 in
    --fix) MODE="fix"; FIX_FLAG="-i"; shift ;;
    *) echo "Unknown parameter: $1"; exit 1 ;;
  esac
done

echo "Running linters in $MODE mode..."

# Source directories to lint
SOURCE_DIRS=(
  "Controller/src"
  "Controller/inc"
  "Middleware/src"
  "Middleware/inc"
  "zmq/src"
  "zmq/inc"
  "modules/cluster-display/ClusterDisplay/src"
  "modules/cluster-display/ClusterDisplay/inc"
)

# Include directories for clang-tidy
INCLUDE_DIRS=(
  "-IController/inc"
  "-IMiddleware/inc"
  "-Izmq/inc"
  "-Imodules/cluster-display/ClusterDisplay/inc"
)

# Run clang-format
echo "Running clang-format..."
FOUND_FILES=0
for dir in "${SOURCE_DIRS[@]}"; do
  if [ -d "$dir" ]; then
    FILES=$(find "$dir" -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.cc' \))
    if [ -n "$FILES" ]; then
      FOUND_FILES=1
      echo "Formatting files in $dir..."
      echo "$FILES" | xargs clang-format $FIX_FLAG
    fi
  else
    echo "Warning: Directory $dir not found, skipping."
  fi
done

if [ $FOUND_FILES -eq 0 ]; then
  echo "No source files found to format!"
  exit 1
fi

# Run clang-tidy if not in fix mode (clang-tidy fix can be risky in CI)
echo "Running clang-tidy..."
FOUND_FILES=0
for dir in "${SOURCE_DIRS[@]}"; do
  if [ -d "$dir" ]; then
    FILES=$(find "$dir" -name '*.cpp' -o -name '*.cc')
    if [ -n "$FILES" ]; then
      FOUND_FILES=1
      echo "Checking files in $dir..."
      echo "$FILES" | xargs -I{} clang-tidy {} -- ${INCLUDE_DIRS[@]} || true
    fi
  fi
done

if [ $FOUND_FILES -eq 0 ]; then
  echo "No source files found for clang-tidy!"
  exit 1
fi

echo "Linting completed successfully!"
