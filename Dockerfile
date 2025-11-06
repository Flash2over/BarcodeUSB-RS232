FROM ubuntu:22.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y     git cmake build-essential gcc-arm-none-eabi libnewlib-arm-none-eabi     python3 && rm -rf /var/lib/apt/lists/*

WORKDIR /opt
RUN git clone https://github.com/raspberrypi/pico-sdk.git &&     cd pico-sdk && git submodule update --init

ENV PICO_SDK_PATH=/opt/pico-sdk
WORKDIR /work

# Build example:
# docker build -t pico-fw .
# docker run --rm -v "$PWD:/work" pico-fw /bin/bash -lc "mkdir -p build && cd build && cmake .. -DPICO_BOARD=pico2 && make -j"
