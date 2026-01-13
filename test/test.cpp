// Copyright 2025 OUXT Polaris.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include <proto_files/conversion_std_msgs__String.hpp>
#include <protolink/client.hpp>
#include <protolink/serial_protocol.hpp>

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

TEST(UDP, send_proto)
{
  // std::shared_ptr<boost::asio::io_context> io =
  // std::make_shared<boost::asio::io_context>();
  protolink::IoContext ioc;
  std::shared_ptr<protolink::udp_protocol::soket> sock =
    protolink::udp_protocol::create_socket(ioc, 8000);
  auto client = protolink::udp_protocol::Publisher<protolink__std_msgs__String::std_msgs__String>(
    sock, "127.0.0.1", 8000);
  // protolink__std_msgs__String::std_msgs__String string_msg;
  // string_msg.set_data("Hello World");
  // client.send(string_msg);
}

// TEST(UDP, recieve_proto)
// {
//   protolink::IoContext ioc;
//   auto client =
//   protolink::udp_protocol::Subscriber<protolink__std_msgs__String::std_msgs__String>(
//     ioc.get(), 8000, [](const auto &) {});
// }

// TEST(MQTT, connect) { protolink::mqtt_protocol::Publisher("127.0.0.1",
// "protolink", "hello", 1); }

TEST(Serial, buildtest_publisher)
{
  /// This test case may access a device file that does not exist, so only
  /// verify that it can be built
  try {
    protolink::IoContext ioc;
    std::shared_ptr<protolink::serial_protocol::port> port =
      protolink::serial_protocol::create_port(ioc, "/dev/ttyACM0", 9600);
    protolink::serial_protocol::Publisher<protolink__std_msgs__String::std_msgs__String> publisher(
      port);
  } catch (...) {
  }
}

TEST(Serial, buildtest_subscriber)
{
  /// This test case may access a device file that does not exist, so only
  /// verify that it can be built
  try {
    protolink::IoContext ioc;
    std::shared_ptr<protolink::serial_protocol::port> port =
      protolink::serial_protocol::create_port(ioc, "/dev/ttyACM0", 9600);
    protolink::serial_protocol::Subscriber<protolink__std_msgs__String::std_msgs__String, 256>
      subscriber(port, [](const auto &) {});
  } catch (...) {
  }
}

using AdaptedType =
  rclcpp::TypeAdapter<protolink__std_msgs__String::std_msgs__String, std_msgs::msg::String>;

class PubNode : public rclcpp::Node
{
public:
  explicit PubNode(const rclcpp::NodeOptions & options) : Node("test", options)
  {
    publisher_ = create_publisher<AdaptedType>("string", 1);
  }
  void publish(const protolink__std_msgs__String::std_msgs__String & proto)
  {
    publisher_->publish(proto);
  }

private:
  std::shared_ptr<rclcpp::Publisher<AdaptedType>> publisher_;
};

class SubNode : public rclcpp::Node
{
public:
  explicit SubNode(
    const rclcpp::NodeOptions & options,
    const std::function<void(const protolink__std_msgs__String::std_msgs__String &)> function)
  : Node("test", options)
  {
    subscriber_ = create_subscription<AdaptedType>("string", 1, function);
  }

private:
  std::shared_ptr<rclcpp::Subscription<AdaptedType>> subscriber_;
};

TEST(TypeAdapter, pub_sub)
{
  rclcpp::init(0, nullptr);
  rclcpp::NodeOptions options;
  options.use_intra_process_comms(true);
  bool proto_recieved = false;
  auto sub_node = std::make_shared<SubNode>(
    options, [&](const protolink__std_msgs__String::std_msgs__String & proto) {
      EXPECT_STREQ("Hello!", proto.data().c_str());
      proto_recieved = true;
    });
  auto pub_node = std::make_shared<PubNode>(options);
  protolink__std_msgs__String::std_msgs__String proto;
  proto.set_data("Hello!");
  pub_node->publish(proto);
  rclcpp::executors::SingleThreadedExecutor exec;
  exec.add_node(sub_node);
  exec.add_node(pub_node);
  exec.spin_some();
  rclcpp::shutdown();
  EXPECT_TRUE(proto_recieved);
}
