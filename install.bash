#!/bin/bash

root=$(pwd)

if ! uv sync; then
    echo "Error: uv python environment not found (uv sync failed)." >&2
    exit 1
fi

############################################
## Install dependencies and Setup python env
############################################
sudo apt update
sudo apt install -y libboost-all-dev libpaho-mqtt-dev libpaho-mqttpp-dev cmake ninja-build
uv add protobuf grpcio-tools jinja2

###############################
## Build and Install abseil-cpp
###############################
git clone https://github.com/abseil/abseil-cpp.git ${root}/abseil-cpp && cd ${root}/abseil-cpp
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local -DABSL_ENABLE_INSTALL=ON -DCMAKE_POSITION_INDEPENDENT_CODE=ON
sudo cmake --build build --target install --parallel 20

#############################
## Build and Install protobuf
#############################
git clone -b v31.1 https://github.com/protocolbuffers/protobuf.git ${root}/protobuf && cd ${root}/protobuf
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_SHARED_LIBS=ON -DCMAKE_POSITION_INDEPENDENT_CODE=ON
cmake --build build --parallel 20
sudo cmake --install build

###########################
## Set Environment Variable
###########################
echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib' >>~/.bashrc
