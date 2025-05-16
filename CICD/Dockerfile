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

    # Update ZeroMQ to latest version
    && git clone https://github.com/zeromq/cppzmq.git \
    && cd cppzmq && mkdir build && cd build \
    && cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local \
    && make install \

    # Clean up the build and apt cache
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* \
    && rm -rf /cppzmq
