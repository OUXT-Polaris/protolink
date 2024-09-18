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

#include <rclcpp/rclcpp.hpp>

template <typename ProtoMessage, typename ROS2Message>
struct rclcpp::TypeAdapter<
  ProtoMessage, ROS2Message,
  typename std::enable_if<rosidl_generator_traits::is_message<ROS2Message>::value>::type>
{
  using is_specialized = std::true_type;
  using custom_type = ProtoMessage;
  using ros_message_type = ROS2Message;

  static void convert_to_ros_message(const custom_type & source, ros_message_type & destination) {}

  static void convert_to_custom(const ros_message_type & source, custom_type & destination) {}
};
