ARG ros_distro="jazzy"

FROM ros:${ros_distro}

ENV DEBIAN_FRONTEND=noninteractive

SHELL ["/bin/bash", "-c"]

###############################
## Build and Install abseil-cpp
###############################
RUN git clone https://github.com/abseil/abseil-cpp.git /source/abseil-cpp && \
    cd /source/abseil-cpp && \
    cmake -S . -B build  -DCMAKE_INSTALL_PREFIX=/usr/local -DABSL_ENABLE_INSTALL=ON  -DCMAKE_POSITION_INDEPENDENT_CODE=ON && \
    cmake --build build --target install --parallel 20

#############################
## Build and Install protobuf
#############################
RUN git clone -b v31.1 https://github.com/protocolbuffers/protobuf.git /source/protobuf && \
    cd /source/protobuf && \
    git submodule update --init --recursive && \
    cmake -S . -B build   -DCMAKE_INSTALL_PREFIX=/usr/local   -DCMAKE_BUILD_TYPE=Release -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_SHARED_LIBS=ON -DCMAKE_POSITION_INDEPENDENT_CODE=ON && \
    cmake --build build --parallel 20 && \
    cmake --install build

############################################
## Install dependencies and Setup python env
############################################
RUN apt-get update && \
    apt-get install -y libboost-all-dev libpaho-mqtt-dev libpaho-mqttpp-dev cmake ninja-build python3 python3-venv python3-pip python3-jinja2 && \
    python3 -m venv /.venv --system-site-packages && \
    source /.venv/bin/activate && \
    echo "source /.venv/bin/activate" >> ~/.bashrc && \
    python3 -m pip install protobuf grpcio-tools

WORKDIR /home/ros2_ws

#######################
## Create ROS workspace
#######################
RUN mkdir -p /home/ros2_ws/src && \
    echo "source /home/ros2_ws/install/setup.bash" >> ~/.bashrc && \
    echo "source /opt/ros/${ROS_DISTRO}/setup.bash" >> ~/.bashrc


RUN git clone -b 24-feat-dockerfile https://github.com/OUXT-Polaris/protolink.git /home/ros2_ws/src/protolink && \
    source /ros_entrypoint.sh && \
    source /.venv/bin/activate && \
    colcon build

ENTRYPOINT [ "/bin/bash", "-c", "/bin/bash"]
