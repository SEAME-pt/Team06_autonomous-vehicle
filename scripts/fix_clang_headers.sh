#!/bin/bash
# Script to fix clang-tidy include path issues on CI

set -e

echo "Fixing clang include paths for CI environment..."

# Create directories for clang includes
mkdir -p /usr/lib/clang/6.0/include
mkdir -p /usr/lib/clang/10.0.0/include

# Copy standard header files to clang include directories
STDDEF_FOUND=false

# Try to find stddef.h in standard locations
for search_path in "/usr/include/stddef.h" "/usr/include/linux/stddef.h" "/usr/lib/gcc/aarch64-linux-gnu/7.5.0/include/stddef.h"; do
  if [ -f "$search_path" ]; then
    echo "Found stddef.h at $search_path"
    echo "Copying stddef.h to clang include directories"
    cp -f "$search_path" /usr/lib/clang/6.0/include/
    cp -f "$search_path" /usr/lib/clang/10.0.0/include/
    STDDEF_FOUND=true
    break
  fi
done

if [ "$STDDEF_FOUND" = false ]; then
  echo "Warning: stddef.h not found in standard locations"
  echo "Searching for stddef.h in the entire system..."
  STDDEF_PATH=$(find /usr -name stddef.h | head -n 1)
  if [ -n "$STDDEF_PATH" ]; then
    echo "Found stddef.h at $STDDEF_PATH"
    cp -f "$STDDEF_PATH" /usr/lib/clang/6.0/include/
    cp -f "$STDDEF_PATH" /usr/lib/clang/10.0.0/include/
    STDDEF_FOUND=true
  else
    echo "ERROR: stddef.h not found anywhere on the system"
    echo "Creating a minimal stddef.h as a last resort"
    cat > /usr/lib/clang/6.0/include/stddef.h << 'EOF'
#ifndef _STDDEF_H
#define _STDDEF_H

#ifndef __cplusplus
#define NULL ((void *)0)
#else
#define NULL 0
#endif

typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef int wchar_t;

#define offsetof(type, member) __builtin_offsetof(type, member)

#endif /* _STDDEF_H */
EOF
    cp -f /usr/lib/clang/6.0/include/stddef.h /usr/lib/clang/10.0.0/include/
  fi
fi

# Do the same for other standard headers
for header in "stdarg.h" "stdbool.h"; do
  HEADER_FOUND=false
  for search_path in "/usr/include/$header" "/usr/include/linux/$header" "/usr/lib/gcc/aarch64-linux-gnu/7.5.0/include/$header"; do
    if [ -f "$search_path" ]; then
      echo "Found $header at $search_path"
      echo "Copying $header to clang include directories"
      cp -f "$search_path" /usr/lib/clang/6.0/include/
      cp -f "$search_path" /usr/lib/clang/10.0.0/include/
      HEADER_FOUND=true
      break
    fi
  done

  if [ "$HEADER_FOUND" = false ]; then
    echo "Warning: $header not found in standard locations"
  fi
done

# Find all GCC include paths
echo "Finding GCC include paths..."
GCC_INCLUDE_PATHS=$(gcc -xc++ -E -v - < /dev/null 2>&1 | grep "^ " | tr "\n" ":")

# Export GCC include paths for clang-tidy to use
export CPLUS_INCLUDE_PATH="$GCC_INCLUDE_PATHS"
echo "Exported CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH"

# Try two approaches to fix clang-tidy

# Approach 1: Create a wrapper script
echo "Creating clang-tidy wrapper..."
cat > /usr/local/bin/clang-tidy-wrapper << 'EOF'
#!/bin/bash
export CPLUS_INCLUDE_PATH="$CPLUS_INCLUDE_PATH"
export C_INCLUDE_PATH="/usr/lib/clang/6.0/include:/usr/include:/usr/local/include"
exec clang-tidy "$@"
EOF
chmod +x /usr/local/bin/clang-tidy-wrapper

# Try to use the wrapper
if [ -x /usr/local/bin/clang-tidy-wrapper ]; then
  echo "Setting up clang-tidy alias..."
  alias clang-tidy=/usr/local/bin/clang-tidy-wrapper
  echo "alias clang-tidy=/usr/local/bin/clang-tidy-wrapper" >> ~/.bashrc
else
  echo "Warning: Failed to create clang-tidy wrapper"
fi

# Approach 2: Try to modify clang's resource directory (more direct)
echo "Attempting to find and fix clang's resource directory..."
CLANG_VERSION=$(clang --version | grep -oP 'clang version \K[0-9.]+' || echo "unknown")
echo "Detected clang version: $CLANG_VERSION"

if [ "$CLANG_VERSION" != "unknown" ]; then
  CLANG_RESOURCE_DIR=$(clang -print-resource-dir 2>/dev/null || echo "")
  if [ -n "$CLANG_RESOURCE_DIR" ] && [ -d "$CLANG_RESOURCE_DIR" ]; then
    echo "Found clang resource directory at $CLANG_RESOURCE_DIR"
    mkdir -p "$CLANG_RESOURCE_DIR/include"

    # Copy our fixed stddef.h if we found/created it
    if [ -f "/usr/lib/clang/6.0/include/stddef.h" ]; then
      cp -f /usr/lib/clang/6.0/include/stddef.h "$CLANG_RESOURCE_DIR/include/"
      echo "Copied stddef.h to $CLANG_RESOURCE_DIR/include/"
    fi
  else
    echo "Warning: Could not determine clang resource directory"
  fi
fi

echo "Clang header fixes completed."
