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

#include <protolink/client.hpp>

namespace protolink
{
namespace udp_protocol
{
Publisher::Publisher(
  boost::asio::io_service & io_service, const std::string & ip_address, const uint16_t port,
  const uint16_t from_port, const rclcpp::Logger & logger)
: endpoint(boost::asio::ip::udp::endpoint(
    boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(ip_address), port))),
  logger(logger),
  sock_(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), from_port))
{
}

void Publisher::sendEncodedText(const std::string & encoded_text)
{
  sock_.send_to(boost::asio::buffer(encoded_text), endpoint);
}
}  // namespace udp_protocol


// namespace mqtt_protocol
// {
// Publisher::Publisher(
//   const std::string & server_address, const std::string & client_id, const std::string & topic,
//   const int qos, const rclcpp::Logger & logger)
// : topic(topic),
//   qos(qos),
//   logger(logger),
//   client_impl_(server_address, client_id, "/tmp/mqtt"),
//   connection_thread_running_(true)
// {
//   client_impl_.set_callback(callback_);
//   auto connect_options = mqtt::connect_options_builder()
//                            .clean_session()
//                            .will(mqtt::message(topic, LWT_PAYLOAD, strlen(LWT_PAYLOAD), qos, false))
//                            .finalize();
//   connect_token_ = client_impl_.connect(connect_options);
//   connection_thread_ = std::thread([this]() {
//     const auto connect = [this]() {
//       try {
//         if (!client_impl_.is_connected()) {
//           mqtt::token_ptr conntok = client_impl_.connect();
//           conntok->wait();
//         }
//       } catch (const mqtt::exception & exc) {
//         RCLCPP_ERROR_STREAM(this->logger, exc.what());
//       }
//     };
//     while (connection_thread_running_) {
//       connect();
//       std::this_thread::sleep_for(std::chrono::seconds(1));
//     }
//   });
// }

// Publisher::~Publisher()
// {
//   connection_thread_running_ = false;
//   connection_thread_.join();
// }

// void Publisher::sendEncodedText(const std::string & encoded_text)
// {
//   if (client_impl_.is_connected()) {
//     client_impl_.publish(topic, encoded_text.c_str(), encoded_text.size(), qos, false);
//   }
// }

// Subscriber::Subscriber(
//   const std::string & server_address, const std::string & client_id, const std::string & topic,
//   const int qos, const rclcpp::Logger & logger)
// : topic(topic), qos(qos), logger(logger), client_impl_(server_address, client_id)
// {
// }
// }  // namespace mqtt_protocol
}  // namespace protolink
