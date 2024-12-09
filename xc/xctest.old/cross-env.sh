#!/bin/bash

function install() {
    cat > Dockerfile << 'EOL'
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential cmake git python3 python3-pip \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu libopencv-dev \
    && rm -rf /var/lib/apt/lists/*
ENV CROSS_COMPILE=aarch64-linux-gnu-
ENV CC=${CROSS_COMPILE}gcc
ENV CXX=${CROSS_COMPILE}g++
WORKDIR /workspace
EOL
    docker build -t cross-env .
}

function run() {
    docker run -it -v $(pwd):/workspace cross-env /bin/bash
}

function clean() {
    docker stop $(docker ps -a -q) 2>/dev/null || true
    docker rm $(docker ps -a -q) 2>/dev/null || true
}

function purge() {
    clean
    docker system prune -af --volumes
    rm -f Dockerfile
}

case "$1" in
    "install") install ;;
    "run") run ;;
    "clean") clean ;;
    "purge") purge ;;
    *) echo "Usage: $0 {install|run|clean|purge}" ;;
esac
