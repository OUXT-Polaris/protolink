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

#include <mqtt/async_client.h>

#include <boost/asio.hpp>
#include <rclcpp/rclcpp.hpp>
#include <thread>

namespace protolink
{
namespace udp_protocol
{
class Client
{
public:
  explicit Client(const std::string & ip_address, const uint16_t port);
  const boost::asio::ip::udp::endpoint endpoint;

  template <typename Proto>
  void send(const Proto & message)
  {
    std::string encoded_text = "";
    message.SerializeToString(&encoded_text);
    sendEncodedText(encoded_text);
  }

private:
  void sendEncodedText(const std::string & encoded_text);
  boost::asio::io_service io_service_;
  boost::asio::ip::udp::socket sock_;
};
}  // namespace udp_protocol

namespace mqtt_protocol
{
const char * LWT_PAYLOAD = "Last will and testament.";

class Client
{
public:
  explicit Client(
    const std::string & server_address, const std::string & client_id, const std::string & topic,
    const int qos = 1);
  ~Client();

  const std::string topic;
  const int qos;

private:
  void sendEncodedText(const std::string & encoded_text);
  void connect();
  mqtt::async_client client_impl_;
  mqtt::callback callback_;
  mqtt::token_ptr connect_token_;
  std::thread connection_thread_;
  bool connection_thread_running_;
};
}  // namespace mqtt_protocol
}  // namespace protolink
