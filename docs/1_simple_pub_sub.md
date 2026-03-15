# Simple Publish and Subscribe

## Overview

The code used in these examples can be found [here](https://github.com/OUXT-Polaris/protolink_drivers_example/tree/main/protolink_drivers_example/simple_pub_sub).

## 1. Crete ROS2 Workspace and Package

(Optional) If there is no workspace, create one.

```bash
WSROOT=~/ros2_ws
mkdir -p ${WSROOT}/src
```

Create package

```bash
WSROOT=~/ros2_ws  # <- Please specify the path to your ROS2 workspace.
cd ${WSROOT}/src
ros2 pkg create protolink_drivers_simple --build-type ament_cmake --dependencies rclcpp geometry_msgs protolink
```

## 2. Generate proto and nanopb

Add task < `add_protolink_message_from_ros_message("geometry_msgs" "Vector3")` > to CMakeLists.txt

```cmake
find_package(geometry_msgs REQUIRED)
find_package(protolink REQUIRED)

# Add the following
add_protolink_message_from_ros_message("geometry_msgs" "Vector3")
```

Build

```bash
cd ${WSROOT}
colcon build --symlink-install --packages-select protolink_drivers_simple --cmake-args -GNinja
```

(Optional) Checking the generated files

```bash
sudo apt update && sudo apt install tree
tree -L 3 build/protolink_drivers_simple
```

> Terminal Output
> 
> ```txt
> ...
> ├── libgeometry_msgs__Vector3.so
> ├── libgeometry_msgs__Vector3_proto.so
> ├── nanopb_gen
> │   ├── STM32CubeIDE
> │   │   └── proto
> │   └── platformio
> │       └── protolink_msgs
> ├── proto_files
> │   └── geometry_msgs__Vector3.proto
> ...
> ```

## 3. Write ROS2 Node Code

Download the example code by entering the following command:

```bash
wget -O ${WSROOT}/src/protolink_drivers_simple/src/simple_pub_sub.cpp https://raw.githubusercontent.com/OUXT-Polaris/protolink_drivers_example/refs/heads/main/protolink_drivers_example/simple_pub_sub/src/simple_pub_sub.cpp
```

`simple_pub_sub.cpp` will be created. Open it and you will see the following code.
```cpp
#include <proto_files/conversion_geometry_msgs__Vector3.hpp>
#include <protolink/client.hpp>
#include <rclcpp/rclcpp.hpp>

namespace protolink_drivers_example
{

// Network settings
const std::string ip_address = "192.168.0.100";  // of microcontroller, etc.
const uint16_t to_port = 8000;                   // same as above.
const uint16_t from_port = 8000;                 // of the computer executing this code.
const uint16_t sub_port = 9000;                  // same as above

class SimplePubSub : public rclcpp::Node
{
public:
  explicit SimplePubSub()
  : Node("simple_pub_sub"),
    protolink_publisher_(io_context_, ip_address, to_port, from_port, this->get_logger()),
    publish_timer_(create_wall_timer(
      std::chrono::duration<double>(1.0 / 10),
      [&]() {
        std::lock_guard<std::mutex> lock(mutex_);

        auto msg = std::make_unique<geometry_msgs::msg::Vector3>();
        msg->x = count;
        msg->y = count * 1.5;
        msg->z = count * 2.0;
        protolink_publisher_.send(convert(*std::move(msg)));
        RCLCPP_INFO(
          this->get_logger(), "Publish    --->  x: %f  y: %f  z: %f", msg->x, msg->y, msg->z);
      })),
    protolink_subscriber_(io_context_, sub_port, [this](const auto & _msg) {
      std::lock_guard<std::mutex> lock(mutex_);

      geometry_msgs::msg::Vector3 msg = convert(_msg);
      count = msg.x;
      RCLCPP_INFO(
        this->get_logger(), "Subscribe  --->  x: %f  y: %f  z: %f\n", msg.x, msg.y, msg.z);
    })
  {
  }

private:
  boost::asio::io_context io_context_;
  std::mutex mutex_;

  using protoVector3 = protolink__geometry_msgs__Vector3::geometry_msgs__Vector3;

  protolink::udp_protocol::Publisher<protoVector3> protolink_publisher_;
  const rclcpp::TimerBase::SharedPtr publish_timer_;
  protolink::udp_protocol::Subscriber<protoVector3> protolink_subscriber_;

  int count = 0.0;
};
}  // namespace protolink_drivers_example

int main(int argc, char * argv[])
{
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);
  rclcpp::init(argc, argv);

  try {
    auto node = std::make_shared<protolink_drivers_example::SimplePubSub>();
    rclcpp::spin(node);
  } catch (std::exception & e) {
    std::cout << e.what() << std::endl;
  }
  rclcpp::shutdown();
  return 0;
}
```

## 3.1 Examine the code

The top of the code includes the C++ headers you will be using. `protolink/client.hpp` is a protocol client library that communication using UDP or serial. `conversion_geometry_msgs__Vector3.hpp` is a header generated from ROS2 messages by protolink that serializes and deserializes data.
```cpp
#include <proto_files/conversion_geometry_msgs__Vector3.hpp>
#include <protolink/client.hpp>
#include <rclcpp/rclcpp.hpp>
```

The following line defines the network infomation(IP address, port) required for UDP communication. `ip_address` and `to_port` are the IP address and port number of the target device, such as a microcontroller(Teensy), with which you want to communicate with ROS2. `from_port` is the port number of the computer that publishes to the target device. `sub_port` is the port number of the computer used for subscribing from the target device.
```cpp
// Network settings
const std::string ip_address = "192.168.0.100";  // of microcontroller, etc.
const uint16_t to_port = 8000;                   // same as above.
const uint16_t from_port = 8000;                 // of the computer executing this code.
const uint16_t sub_port = 9000;                  // same as above
```

The next line creates the node class `SimplePubSub` by inheriting from `rclcpp::Node`.
```cpp
class SimplePubSub : public rclcpp::Node
```

The public constructor initializes `protolink_publisher_`, `publish_timer_`, and `protolink_subscriver_`. The `protolink_publisher_` sets the I/O service, destination IP address and port, and the source port. The `publish_timer_` is the same as a typical ROS 2 Publisher. The only difference is the final send function, which corresponds to the ROS 2 pub function. It passes data that has been converted from a ROS 2 message into a Protocol Buffers structured data. It's important to note that the convert function does not use a pointer pass. At last, `protolink_subscriver_` sets the I/O service, the receiving port number, and the callback function.
```cpp
public:
  explicit SimplePubSub()
  : Node("simple_pub_sub"),
    protolink_publisher_(io_context_, ip_address, to_port, from_port, this->get_logger()),
    publish_timer_(create_wall_timer(
      std::chrono::duration<double>(1.0 / 10),
      [&]() {
        std::lock_guard<std::mutex> lock(mutex_);

        auto msg = std::make_unique<geometry_msgs::msg::Vector3>();
        msg->x = count;
        msg->y = count * 1.5;
        msg->z = count * 2.0;
        protolink_publisher_.send(convert(*std::move(msg)));
        RCLCPP_INFO(
          this->get_logger(), "Publish    --->  x: %f  y: %f  z: %f", msg->x, msg->y, msg->z);
      })),
    protolink_subscriber_(io_context_, sub_port, [this](const auto & _msg) {
      std::lock_guard<std::mutex> lock(mutex_);

      geometry_msgs::msg::Vector3 msg = convert(_msg);
      count = msg.x;
      RCLCPP_INFO(
        this->get_logger(), "Subscribe  --->  x: %f  y: %f  z: %f\n", msg.x, msg.y, msg.z);
    })
  {
  }
```

The private section at the end of the class is the declarations of the I/O service, publisher, subscriber, timer, and counter. It resembles ROS 2's publisher and subscriber, doesn't it?
```cpp
private:
  boost::asio::io_context io_context_;
  std::mutex mutex_;

  using protoVector3 = protolink__geometry_msgs__Vector3::geometry_msgs__Vector3;

  protolink::udp_protocol::Publisher<protoVector3> protolink_publisher_;
  const rclcpp::TimerBase::SharedPtr publish_timer_;
  protolink::udp_protocol::Subscriber<protoVector3> protolink_subscriber_;

  int count = 0.0;
```

This is the usual main function. It starts the node.
```cpp
int main(int argc, char * argv[])
{
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);
  rclcpp::init(argc, argv);

  try {
    auto node = std::make_shared<protolink_drivers_example::SimplePubSub>();
    rclcpp::spin(node);
  } catch (std::exception & e) {
    std::cout << e.what() << std::endl;
  }
  rclcpp::shutdown();
  return 0;
}
```

## 3.2 Add dependencies

The following is included in package.xml because the dependencies were specified during package creation. If it's not present, please add it.
```xml
  <depend>rclcpp</depend>
  <depend>geometry_msgs</depend>
  <depend>protolink</depend>
```

## 3.3 Add build task to CMakeLists.txt

Add an executable after `add_protolink_me...`.
```cmake
add_executable(simple_pub_sub
  ./src/simple_pub_sub.cpp
)
```

To link the library generated by protolink, add `geometry_msgs__Vector3_proto` to pubsub as target_link_libraries. The notation format is `{ros2_msgs_namespace}_{ros2_msgs_type}_proto`
```cmake
ament_target_dependencies(simple_pub_sub
  rclcpp
  geometry_msgs
  protolink
)
target_link_libraries(simple_pub_sub
  geometry_msgs__Vector3_proto
)
```

Finaly, add the `install` section.
```cmake
install(TARGETS simple_pub_sub
  DESTINATION lib/${PROJECT_NAME}
)
```

## 3.4 Build and run

Move to the root of your workspace and build the new package.
```bash
cd ${WSROOT}
colcon build --symlink-install --packages-select protolink_drivers_simple --cmake-args -GNinja
```

Source the setup files.
```bash
. install/setup.bash
```

Run pubsub node.
```bash
ros2 run protolink_drivers_simple simple_pub_sub
```

## 4. Write Arduino Code for Teensy

This tutorial uses PlatformIO to build and upload Arduino code. Therefore, the folder structure is also dependent on it. Various files are available for download from [here](https://github.com/OUXT-Polaris/protolink_drivers_example/tree/main/firmware/simple_pub_sub).

The folder structure is as follows.
```txt
├── lib
│   ├── protolink_msgs
│       └── proto
│           ├── geometry_msgs__Vector3.pb.c
│           ├── geometry_msgs__Vector3.pb.h
│           └── geometry_msgs__Vector3.proto
├── src
│   └── main.cpp
└── platformio.ini
```

```cpp
#include <Arduino.h>
#include <NativeEthernetUdp.h>
#include <pb_encode.h>
#include <pb_decode.h>

#include <proto/geometry_msgs__Vector3.pb.h>


byte MAC[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAD};
const IPAddress IP(192, 168, 0, 101);
const unsigned int PORT = 8000;  // port to listen on

const IPAddress DIST_IP(192, 168, 0, 100);
const unsigned int DIST_PORT = 9000;


using Vector3 = protolink__geometry_msgs__Vector3_geometry_msgs__Vector3;

EthernetUDP Udp;
Vector3 msg = protolink__geometry_msgs__Vector3_geometry_msgs__Vector3_init_zero;


void publish()
{
  msg.x *= 2;
  msg.y *= 2;
  msg.z *= 2;

  uint8_t packetBuffer[UDP_TX_PACKET_MAX_SIZE];
  pb_ostream_t stream = pb_ostream_from_buffer(packetBuffer, sizeof(packetBuffer));
  bool result = pb_encode(&stream, protolink__geometry_msgs__Vector3_geometry_msgs__Vector3_fields, &msg);

  if (result)
  {
      Udp.beginPacket(DIST_IP, DIST_PORT);
      Udp.write(packetBuffer, stream.bytes_written);
      Udp.endPacket();
      Serial.printf("written bytes: %d,", stream.bytes_written);
  }
  else {
      Serial.printf("Encode error\n");
  }
}


void subscribe()
{
  uint8_t packetBuffer[UDP_TX_PACKET_MAX_SIZE];
  int num_bytes = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
  pb_istream_t pb_stream = pb_istream_from_buffer(packetBuffer, num_bytes);
  bool decode_res = pb_decode(&pb_stream, protolink__geometry_msgs__Vector3_geometry_msgs__Vector3_fields, &msg);

  if (decode_res) {
    Serial.printf("Subscribe data --> x: %lf,  y: %lf,  z:%lf\n", msg.x, msg.y, msg.z);
  }
  else {
    Serial.printf("Decode error\n");
  }
}


void setup()
{
  Serial.begin(9600);

  pinMode(13, OUTPUT);

  Ethernet.begin(MAC, IP);
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // Start UDP
  Udp.begin(PORT);
}


void loop()
{
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    // Subscribe
    subscribe();

    // Publish
    publish();
  }
  delay(10);
}
```

## 4.1 Copy the library generated by nanopb

Copy the files generated in [2. Generate .proto and API]() to `lib`．The files are `geometry_msgs__Vector3.pb.c`, `geometry_msgs__Vector3.pb.h`, and `geometry_msgs__Vector3.proto`．Although `geometry_msgs__Vector3.proto` is not used, it is recommended to copy is since it contains the message definition. The file path is as follows.
```bash
# ${WSROOT}/ros2_ws/build/protolink_drivers_simple/nanopb_gen/platformio/protolink_msgs
cp -r ${WSROOT}/ros2_ws/build/protolink_drivers_simple/nanopb_gen/platformio/protolink_msgs lib/
```

## 4.2 Examine code

The top of the code includes the C++ headers you will be using. `NativeEthernetUdp.h` enable the use of Ethernet on Teensy. `pb_encode.h` and `pb_decode.h` convert between proto-formated messages and binary data. `proto/geometry_msgs__Vector3.pb.` is defined geometry_msgs/Vector3 messages.
```cpp
#include <Arduino.h>
#include <NativeEthernetUdp.h>
#include <pb_encode.h>
#include <pb_decode.h>

#include <proto/geometry_msgs__Vector3.pb.h>
```

The following line defines the network infomation(IP address, port) required for UDP communication. `MAC`, `IP`, and `PORT` are Teensy's MAC address, IP address, and utilized port. `DISP_IP` and `DIST_PORT` are the IP address and port number of the destination device, such as a PC running ROS2.
```cpp
byte MAC[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAD};
const IPAddress IP(192, 168, 0, 101);
const unsigned int PORT = 8000;  // port to listen on

const IPAddress DIST_IP(192, 168, 0, 100);
const unsigned int DIST_PORT = 9000;
```

Use `using` to shorten the long type name generated by protolink. 
```cpp
using Vector3 = protolink__geometry_msgs__Vector3_geometry_msgs__Vector3;

EthernetUDP Udp;
Vector3 msg = protolink__geometry_msgs__Vector3_geometry_msgs__Vector3_init_zero;
```


```cpp
void publish()
{
  msg.x *= 2;
  msg.y *= 2;
  msg.z *= 2;

  uint8_t packetBuffer[UDP_TX_PACKET_MAX_SIZE];
  pb_ostream_t stream = pb_ostream_from_buffer(packetBuffer, sizeof(packetBuffer));
  bool result = pb_encode(&stream, protolink__geometry_msgs__Vector3_geometry_msgs__Vector3_fields, &msg);

  if (result)
  {
      Udp.beginPacket(DIST_IP, DIST_PORT);
      Udp.write(packetBuffer, stream.bytes_written);
      Udp.endPacket();
      Serial.printf("written bytes: %d,", stream.bytes_written);
  }
  else {
      Serial.printf("Encode error\n");
  }
}
```


```cpp
void subscribe()
{
  uint8_t packetBuffer[UDP_TX_PACKET_MAX_SIZE];
  int num_bytes = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
  pb_istream_t pb_stream = pb_istream_from_buffer(packetBuffer, num_bytes);
  bool decode_res = pb_decode(&pb_stream, protolink__geometry_msgs__Vector3_geometry_msgs__Vector3_fields, &msg);

  if (decode_res) {
    Serial.printf("Subscribe data --> x: %lf,  y: %lf,  z:%lf\n", msg.x, msg.y, msg.z);
  }
  else {
    Serial.printf("Decode error\n");
  }
}
```


```cpp
void setup()
{
  Serial.begin(9600);

  pinMode(13, OUTPUT);

  Ethernet.begin(MAC, IP);
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // Start UDP
  Udp.begin(PORT);
}
```


```cpp
void loop()
{
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    subscribe();
    publish();
  }
  delay(10);
}
```


## Reference
- [Nanopb](https://github.com/nanopb/nanopb)
- [Teensy Ethernet](https://www.pjrc.com/store/ethernet_kit.html)
