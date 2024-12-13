# missing qt libraries

FROM balenalib/jetson-nano-ubuntu:bionic

RUN apt-get update && apt-get install -y \
    apt-utils \
    build-essential \
    cmake \
    libgpiod-dev \
    libi2c-dev \
    # libgl1-mesa-dev \
    # qtbase5-dev \
    # qtbase5-dev-tools \
    # qtchooser \
    # qtdeclarative5-dev \
    # qml-module-qtquick2 \
    # qml-module-qtquick-layouts \
    # libqt5gui5 \
    # libqt5qml5 \
    # libqt5quick5 \
    # libqt5quickcontrols2-5 \
    # qml-module-qtqml \
    # qml-module-qtquick-controls \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# WORKDIR /repo
