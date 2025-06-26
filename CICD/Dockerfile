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
    lcov \
    gcovr \

    # # Qt5 dependencies
    # qtbase5-dev \
    # qtdeclarative5-dev \
    # qml-module-qtquick2 \
    # qml-module-qtquick-window2 \
    # qml-module-qtquick-layouts \

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

    # Clean up the build and apt cache
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* \
    && rm -rf /tmp/cppzmq-4.8.1 /tmp/v4.8.1.tar.gz
