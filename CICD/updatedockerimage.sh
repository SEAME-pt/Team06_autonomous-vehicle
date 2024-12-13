#!/bin/bash
docker run --rm --privileged multiarch/qemu-user-static --reset -p yes
docker build --rm -t jmoreiraseame/jetson-nano-ubuntu:bionic --platform Linux/ARM64 .
docker push jmoreiraseame/jetson-nano-ubuntu:bionic
