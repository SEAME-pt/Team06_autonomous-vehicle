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

    # Test coverage tools
    lcov \
    gcovr \

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

    # Install cppzmq (ZMQ C++ bindings)
    && cd /tmp \
    && wget https://github.com/zeromq/cppzmq/archive/v4.7.1.zip \
    && unzip v4.7.1.zip \
    && cd cppzmq-4.7.1 \
    && mkdir build && cd build \
    && cmake .. \
    && make -j$(nproc) \
    && make install \
    && ldconfig \

    # Clean up the build and apt cache
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* \
    && rm -rf /tmp/cppzmq-4.7.1 /tmp/v4.7.1.zip
