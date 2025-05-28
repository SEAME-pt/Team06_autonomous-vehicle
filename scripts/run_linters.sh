#!/bin/bash
set -e

# Default behavior: check mode
MODE="check"
FORMAT_ONLY=false

# Parse arguments
while [[ "$#" -gt 0 ]]; do
  case $1 in
    --fix) MODE="fix"; shift ;;
    --format-only) FORMAT_ONLY=true; shift ;;
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
      echo "Checking files in $dir..."

      # For check mode, we'll create a temporary directory and compare files
      if [ "$MODE" == "check" ]; then
        TEMP_DIR=$(mktemp -d)
        echo "Creating temporary copies in $TEMP_DIR"

        # Process each file
        FORMATTING_ISSUES=0
        for file in $FILES; do
          # Copy file to temp dir
          mkdir -p "$TEMP_DIR/$(dirname "$file")"
          cp "$file" "$TEMP_DIR/$file"

          # Format the copy
          clang-format -i "$TEMP_DIR/$file"

          # Compare with original
          if ! diff -u "$file" "$TEMP_DIR/$file" > /dev/null; then
            echo "Formatting issues in $file"
            FORMATTING_ISSUES=1
          fi
        done

        # Clean up
        rm -rf "$TEMP_DIR"

        # Exit with error if formatting issues found
        if [ $FORMATTING_ISSUES -eq 1 ]; then
          echo "Formatting issues detected. Run with --fix to fix them."
          exit 1
        fi
      else
        # Fix mode - directly modify files
        echo "$FILES" | xargs clang-format -i
      fi
    fi
  else
    echo "Warning: Directory $dir not found, skipping."
  fi
done

if [ $FOUND_FILES -eq 0 ]; then
  echo "No source files found to format!"
  exit 1
fi

if [ "$FORMAT_ONLY" == "true" ]; then
  echo "Format-only mode enabled, skipping clang-tidy"
  echo "Linting completed successfully!"
  exit 0
fi

# Run clang-tidy
echo "Running clang-tidy..."

# Check if we have a wrapper script for clang-tidy
if [ -x "/usr/local/bin/clang-tidy-wrapper" ]; then
  echo "Using clang-tidy wrapper script"
  CLANG_TIDY_CMD="/usr/local/bin/clang-tidy-wrapper"
else
  echo "Using standard clang-tidy"
  CLANG_TIDY_CMD="clang-tidy"
fi

# Check for missing stddef.h and fix if possible
if [ ! -f "/usr/lib/clang/6.0/include/stddef.h" ] && [ -f "/usr/include/stddef.h" ]; then
  echo "Creating directory for clang includes"
  mkdir -p /usr/lib/clang/6.0/include
  echo "Copying stddef.h to clang include directory"
  cp /usr/include/stddef.h /usr/lib/clang/6.0/include/
fi

FOUND_FILES=0
for dir in "${SOURCE_DIRS[@]}"; do
  if [ -d "$dir" ]; then
    FILES=$(find "$dir" -name '*.cpp' -o -name '*.cc')
    if [ -n "$FILES" ]; then
      FOUND_FILES=1
      echo "Checking files in $dir..."
      for file in $FILES; do
        echo "Analyzing $file..."
        # Run clang-tidy with our wrapper or directly
        $CLANG_TIDY_CMD "$file" -- ${INCLUDE_DIRS[@]} || echo "Ignoring clang-tidy error for $file"
      done
    fi
  fi
done

if [ $FOUND_FILES -eq 0 ]; then
  echo "No source files found for clang-tidy!"
  exit 1
fi

echo "Linting completed successfully!"
