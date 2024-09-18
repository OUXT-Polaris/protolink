// Copyright 2024 OUXT Polaris.
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
#include <std_msgs__String.pb.h>

#include <protolink/client.hpp>
#include <std_msgs/msg/string.hpp>

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

TEST(UDP, send_proto)
{
  auto client = protolink::udp_protocol::Client("127.0.0.1", 8000);
  protolink__std_msgs__String::std_msgs__String string_msg;
  string_msg.set_data("Hello World");
  client.send(string_msg);
}

TEST(MQTT, connect) { protolink::mqtt_protocol::Client("127.0.0.1", "protolink", "hello"); }
