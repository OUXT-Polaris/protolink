# protolink

## Introduction

Automatic [Protocol Buffers](https://github.com/protocolbuffers/protobuf) generation tool to link ROS2 and microcontroller. The protocol buffer developed by google is used to provide ros2-independent communication.

## Features

- Generate .proto file from ROS2 message
- Generate C++ protocol buffer API from .proto file
- Generate ANSI-C protocol buffer API from .proto file using [Nanopb](https://github.com/nanopb/nanopb)
- Generate function to interconversion between ROS2 message and Protocol Buffers
- Supports custom messages

## Requirements

- ROS2 (humble or latest)
- Python3
- Protocol Buffer (v31.1 or latest) → [Build and Install example](https://github.com/OUXT-Polaris/protolink/blob/master/docs/install_protobuf.md)

## Installation

Install dependencies

```bash
sudo apt update
sudo apt install -y libboost-all-dev libpaho-mqtt-dev libpaho-mqttpp-dev cmake ninja-build python3 python3-venv python3-pip python3-jinja2
```

Install python packages

```bash
python3 -m pip install protobuf grpcio-tools
```

(Python3.12 or latest) Create python virtual env and Install python packages

```bash
python3 -m venv .venv --system-site-packages
source .venv/bin/activate
python3 -m pip install protobuf grpcio-tools
```

Clone the repository to the ros workspace(eg: ros2_ws/src).

```bash
cd {your_ros_workspace_path: ~/ros2_ws/src}
git clone https://github.com/OUXT-Polaris/protolink.git
```

Build package

```bash
cd {your_ros_workspace_path: ~/ros2_ws}
colcon build --symlink-install --cmake-args -GNinja --packages-select protolink
```

## QuickStart

Sample code for communication between a microcomputer and a PC similar to ROS2 topics
PC <--Ethernet--> MCU(Use Teensy4.1 in Sample)

1. [Simple Publish and Subscribe](/docs/1_simple_pub_sub.md)
2. [Using custom message]

## How to Using (only generate)

> [!NOTE]
> It is recommended to create an external repository like [protolink_drivers](https://github.com/OUXT-Polaris/protolink_drivers) and add the protolink and geographic_msgs dependencies there.

When using geographic_msgs/GeoPose

Add denendent packages to package.xml

```xml
# <depend> .....
<depend>geographic_msgs</depend>
```

Add task to CMakeLists.txt

```txt
# include(cmake/add_protolink_message.cmake)....
add_protolink_message_from_ros_message("geographic_msgs" "GeoPose")
```

Generate .proto file and API

```bash
$ cmake -S . -B build -G "Ninja"
$ cmake --build build
```

Path of the generated API file

- C++ API: build/...
- ANSI-C API: build/nanopb-gen/...
- proto file: build/proto_file/...
