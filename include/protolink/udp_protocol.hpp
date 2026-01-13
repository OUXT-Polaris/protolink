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

#ifndef PROTOLINK__UDP_PROTOCOL_HPP_
#define PROTOLINK__UDP_PROTOCOL_HPP_

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <rclcpp/rclcpp.hpp>

#include "utils.hpp"

namespace protolink
{
namespace udp_protocol
{

using soket = boost::asio::ip::udp::socket;

inline std::shared_ptr<boost::asio::ip::udp::socket> create_socket(
  protolink::IoContext & io_context, const uint16_t port)
{
  return std::make_shared<boost::asio::ip::udp::socket>(
    io_context.get(), boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port));
}

template <typename Proto>
class Publisher
{
public:
  explicit Publisher(
    std::shared_ptr<boost::asio::ip::udp::socket> sock, const std::string & ip_address,
    const uint16_t port, const rclcpp::Logger & logger = rclcpp::get_logger("protolink_udp"))
  : endpoint([ip_address, port]() {
      if (ip_address == "255.255.255.255") {
        return boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::broadcast(), port);
      }
      return boost::asio::ip::udp::endpoint(
        boost::asio::ip::address::from_string(ip_address), port);
    }()),
    logger(logger),
    sock_(sock)
  {
    if (ip_address == "255.255.255.255" || ip_address.substr(ip_address.length() - 3, 3) == "255") {
      sock_->set_option(boost::asio::ip::udp::socket::reuse_address(true));
      sock_->set_option(boost::asio::socket_base::broadcast(true));
    }
  }
  const boost::asio::ip::udp::endpoint endpoint;
  const rclcpp::Logger logger;

  void send(const Proto & message)
  {
    std::string encoded_text = "";
    message.SerializeToString(&encoded_text);
    sendEncodedText(encoded_text);
  }

private:
  void sendEncodedText(const std::string & encoded_text)
  {
    sock_->send_to(boost::asio::buffer(encoded_text), endpoint);
  }
  std::shared_ptr<boost::asio::ip::udp::socket> sock_;
};

template <typename Proto, int ReceiveBufferSize = 128>
class Subscriber
{
public:
  explicit Subscriber(
    std::shared_ptr<boost::asio::ip::udp::socket> sock, std::function<void(const Proto &)> callback,
    const rclcpp::Logger & logger = rclcpp::get_logger("protolink_serial"))
  : logger(logger), sock_(sock), callback_(callback)
  {
    start_receive();
  }

  void start_receive()
  {
    sock_->async_receive(
      boost::asio::buffer(receive_data_),
      boost::bind(
        &Subscriber::handler, this, boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
  }
  const rclcpp::Logger logger;

private:
  std::shared_ptr<boost::asio::ip::udp::socket> sock_;
  std::function<void(Proto)> callback_;
  std::thread io_thread_;
  boost::array<char, ReceiveBufferSize> receive_data_;

  void handler(const boost::system::error_code & error, size_t bytes_transferred)
  {
    if (error != boost::system::errc::success) {
      RCLCPP_ERROR_STREAM(
        logger, "Error code : " << error.value() << "\nError Message : " << error.message());
      return;
    }
    std::string data(receive_data_.data(), bytes_transferred);
    Proto proto;
    proto.ParseFromString(data);
    callback_(proto);

    // 再度受信を開始
    start_receive();
  }
};
}  // namespace udp_protocol
}  // namespace protolink

#endif  // PROTOLINK__UDP_PROTOCOL_HPP_
