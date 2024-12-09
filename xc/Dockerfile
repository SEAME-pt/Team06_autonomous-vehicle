FROM balenalib/jetson-nano-ubuntu:latest

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /repo
