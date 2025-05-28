FROM balenalib/jetson-nano-ubuntu:bionic

RUN apt-get update && apt-get install -y \
    apt-utils \
    build-essential \
    libc6-dev \
    libpthread-stubs0-dev \
    git \
    curl \
    wget \
    unzip \
    pkg-config \
    libgpiod-dev \
    libi2c-dev \

    # ZeroMQ dependencies
    libzmq3-dev \

    # Linting and coverage tools
    clang-format \
    clang-tidy \
    clang \
    llvm \
    libclang-dev \
    libc++-dev \
    libc++abi-dev \
    lcov \
    gcovr \

    # Additional C/C++ development headers
    libstdc++-8-dev \
    libstdc++-7-dev \
    linux-libc-dev \
    gcc-8 \
    g++-8 \
    gcc-7 \
    g++-7 \

    # Qt5 dependencies
    qtbase5-dev \
    qtdeclarative5-dev \
    qml-module-qtquick2 \
    qml-module-qtquick-window2 \
    qml-module-qtquick-layouts \

    # Install CMake latest version
    && curl -fsSL https://apt.kitware.com/keys/kitware-archive-latest.asc \
    | gpg --dearmor -o /usr/share/keyrings/kitware-archive-keyring.gpg \
    && echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] \
    https://apt.kitware.com/ubuntu/ bionic main" \
    | tee /etc/apt/sources.list.d/kitware.list \
    && apt-get update \
    && apt-get install -y cmake \

    # Install a newer version of cppzmq (>= 4.7.0) for modern API support
    && cd /tmp \
    && wget https://github.com/zeromq/cppzmq/archive/v4.8.1.tar.gz \
    && tar xzf v4.8.1.tar.gz \
    && cd cppzmq-4.8.1 \
    && mkdir build && cd build \
    && cmake .. -DCPPZMQ_BUILD_TESTS=OFF \
    && make install \
    && ldconfig \

    # Set up clang-tidy to find system headers
    && mkdir -p /usr/lib/clang-tidy \
    && echo "Checks: '-*,clang-analyzer-*,modernize-*,performance-*,readability-*,bugprone-*,-modernize-use-trailing-return-type'" > /usr/lib/clang-tidy/.clang-tidy \
    && echo "HeaderFilterRegex: '.*'" >> /usr/lib/clang-tidy/.clang-tidy \

    # Create a basic compile_commands.json for clang-tidy to use
    && mkdir -p /etc/clang-tidy \
    && echo '[]' > /etc/clang-tidy/compile_commands.json \

    # Clean up the build and apt cache
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* \
    && rm -rf /tmp/cppzmq-4.8.1 /tmp/v4.8.1.tar.gz

# Create symbolic links to ensure headers are found
RUN ln -sf /usr/include/c++/8 /usr/include/c++ \
    && ln -sf /usr/include/x86_64-linux-gnu/c++/8 /usr/include/x86_64-linux-gnu/c++ \
    && ln -sf /usr/include/aarch64-linux-gnu/c++/8 /usr/include/aarch64-linux-gnu/c++ \
    # Also set up links for GCC 7.5.0 which is used by the CI runner
    && ln -sf /usr/include/c++/7 /usr/include/c++/7.5.0 \
    && ln -sf /usr/include/aarch64-linux-gnu/c++/7 /usr/include/aarch64-linux-gnu/c++/7.5.0 \
    # Copy stddef.h to ensure it's found
    && mkdir -p /usr/lib/clang/6.0/include \
    && cp -f /usr/include/stddef.h /usr/lib/clang/6.0/include/ \
    # Make sure clang resource directory is properly set up
    && mkdir -p /usr/lib/clang/10.0.0/include \
    && cp -f /usr/include/stddef.h /usr/lib/clang/10.0.0/include/

# Add compiler include paths to clang-tidy
ENV CPLUS_INCLUDE_PATH=/usr/include/c++/7.5.0:/usr/include/c++/8:/usr/include/aarch64-linux-gnu/c++/7.5.0:/usr/include/aarch64-linux-gnu/c++/8:/usr/include/c++/7.5.0/backward:/usr/include/c++/8/backward:/usr/lib/gcc/aarch64-linux-gnu/7.5.0/include:/usr/lib/gcc/aarch64-linux-gnu/8/include:/usr/local/include:/usr/lib/gcc/aarch64-linux-gnu/7.5.0/include-fixed:/usr/lib/gcc/aarch64-linux-gnu/8/include-fixed:/usr/include/aarch64-linux-gnu:/usr/include

ENV C_INCLUDE_PATH=/usr/lib/gcc/aarch64-linux-gnu/7.5.0/include:/usr/lib/gcc/aarch64-linux-gnu/8/include:/usr/local/include:/usr/lib/gcc/aarch64-linux-gnu/7.5.0/include-fixed:/usr/lib/gcc/aarch64-linux-gnu/8/include-fixed:/usr/include/aarch64-linux-gnu:/usr/include

# Add a special fix for clang-tidy when run in CI environment
RUN echo '#!/bin/bash' > /usr/local/bin/clang-tidy-wrapper.sh \
    && echo 'export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH' >> /usr/local/bin/clang-tidy-wrapper.sh \
    && echo 'export C_INCLUDE_PATH=$C_INCLUDE_PATH' >> /usr/local/bin/clang-tidy-wrapper.sh \
    && echo 'clang-tidy -quiet "$@"' >> /usr/local/bin/clang-tidy-wrapper.sh \
    && chmod +x /usr/local/bin/clang-tidy-wrapper.sh \
    && echo 'alias clang-tidy=/usr/local/bin/clang-tidy-wrapper.sh' >> /root/.bashrc
