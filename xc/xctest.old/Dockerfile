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
