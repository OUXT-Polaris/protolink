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
Client::Client(const std::string & ip_address, const uint16_t port)
: endpoint(boost::asio::ip::udp::endpoint(
    boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(ip_address), port))),
  sock_(io_service_, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port))
{
}

void Client::sendEncodedText(const std::string & encoded_text)
{
  sock_.send_to(boost::asio::buffer(encoded_text), endpoint);
}
}  // namespace udp_protocol

namespace mqtt_protocol
{
Client::Client(
  const std::string & server_address, const std::string & client_id, const std::string & topic,
  const int qos)
: client_impl_(server_address, client_id, "/tmp/mqtt")
{
  client_impl_.set_callback(callback_);
  auto connect_options = mqtt::connect_options_builder()
                           .clean_session()
                           .will(mqtt::message(topic, LWT_PAYLOAD, strlen(LWT_PAYLOAD), qos, false))
                           .finalize();
  connect_token_ = client_impl_.connect(connect_options);
}
}  // namespace mqtt_protocol
}  // namespace protolink
