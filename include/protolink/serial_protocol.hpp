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

#ifndef PROTOLINK__SERIAL_PROTOCOL_HPP_
#define PROTOLINK__SERIAL_PROTOCOL_HPP_

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <rclcpp/rclcpp.hpp>

#include "protolink/utils.hpp"

namespace protolink
{
namespace serial_protocol
{

using port = boost::asio::serial_port;

inline std::shared_ptr<boost::asio::serial_port> create_port(
  protolink::IoContext & io_context, const std::string & device_file, const uint32_t & baud_rate)
{
  auto serial = std::make_shared<boost::asio::serial_port>(io_context.get(), device_file);
  serial->set_option(boost::asio::serial_port_base::baud_rate(baud_rate));
  serial->set_option(boost::asio::serial_port_base::character_size(8));
  serial->set_option(
    boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
  serial->set_option(
    boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
  serial->set_option(
    boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));

  return serial;
}

/**
 * @brief Publisher with serial
 */
template <typename Proto>
class Publisher
{
public:
  explicit Publisher(
    std::shared_ptr<boost::asio::serial_port> serial,
    const rclcpp::Logger & logger = rclcpp::get_logger("protolink_serial_pub"))
  : logger(logger), serial_(serial)
  {
  }

  const rclcpp::Logger logger;

  void send(const Proto & message)
  {
    // Serialize protobuf
    std::string raw_str;
    if (!message.SerializeToString(&raw_str)) {
      RCLCPP_ERROR(logger, "Protobuf serialization failed");
      return;
    }

    // COBS encode
    std::vector<uint8_t> raw_bytes(raw_str.begin(), raw_str.end());
    std::vector<uint8_t> encoded = utils::Cobs::encode(raw_bytes);

    boost::asio::write(*serial_, boost::asio::buffer(encoded));
  }

private:
  std::shared_ptr<boost::asio::serial_port> serial_;
};

/**
 * @brief Subscriber with serial
 */
template <typename Proto, int ReadChunkSize = 64>
class Subscriber
{
public:
  explicit Subscriber(
    std::shared_ptr<boost::asio::serial_port> serial, std::function<void(const Proto &)> callback,
    const rclcpp::Logger & logger = rclcpp::get_logger("protolink_serial_sub"))
  : logger(logger), serial_(serial), callback_(callback)
  {
    do_receive();
  }
  const rclcpp::Logger logger;

private:
  std::shared_ptr<boost::asio::serial_port> serial_;
  std::function<void(Proto)> callback_;

  boost::array<char, ReadChunkSize> read_buffer_;

  // Stream buffer
  std::vector<uint8_t> internal_buffer_;

  void do_receive()
  {
    serial_->async_read_some(
      boost::asio::buffer(read_buffer_),
      boost::bind(
        &Subscriber::handler, this, boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
  }

  void handler(const boost::system::error_code & error, size_t bytes_transferred)
  {
    if (error) {
      if (error != boost::asio::error::operation_aborted) {
        RCLCPP_ERROR(logger, "Read error: %s", error.message().c_str());
      }
      return;
    }

    internal_buffer_.insert(
      internal_buffer_.end(), read_buffer_.begin(), read_buffer_.begin() + bytes_transferred);

    // Find splited character(0x00), and split packet
    process_buffer();

    // Next
    do_receive();
  }

  void process_buffer()
  {
    auto it = internal_buffer_.begin();
    while (true) {
      auto delimiter_it = std::find(it, internal_buffer_.end(), 0x00);

      if (delimiter_it == internal_buffer_.end()) {
        // Preventing overflow
        if (internal_buffer_.size() > 1024 * 10) {
          RCLCPP_WARN(logger, "Buffer overflow reset");
          internal_buffer_.clear();
        }
        break;
      }

      size_t packet_len = std::distance(internal_buffer_.begin(), delimiter_it);

      if (packet_len > 0) {
        // COBS decode
        std::vector<uint8_t> decoded = utils::Cobs::decode(internal_buffer_.data(), packet_len);

        // Protobuf parse
        Proto proto;
        std::string str_data(decoded.begin(), decoded.end());
        if (proto.ParseFromString(str_data)) {
          callback_(proto);
        } else {
          RCLCPP_WARN(logger, "Protobuf parse failed");
        }
      }

      // Delete processed data from the buffer
      internal_buffer_.erase(internal_buffer_.begin(), delimiter_it + 1);

      // Reset iterator
      it = internal_buffer_.begin();
    }
  }
};

}  // namespace serial_protocol
}  // namespace protolink

#endif  // PROTOLINK__SERIAL_PROTOCOL_HPP_
