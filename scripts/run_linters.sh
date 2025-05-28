#!/bin/bash
set -e

# Default behavior: check mode
MODE="check"
FIX_FLAG="-i"

# Parse arguments
while [[ "$#" -gt 0 ]]; do
  case $1 in
    --fix) MODE="fix"; shift ;;
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
