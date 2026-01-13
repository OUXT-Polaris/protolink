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

#ifndef PROTOLINK__UTILS_HPP_
#define PROTOLINK__UTILS_HPP_

#include <boost/asio.hpp>
#include <cstdint>
#include <vector>

namespace protolink
{

/**
 * @brief A wrapper class that manages the lifecycle of io_context and its worker thread.
 * @details Instantiating this class as a member variable automatically spawns a background thread
 * for asynchronous operations.
 */
class IoContext
{
public:
  IoContext() : work_guard_(boost::asio::make_work_guard(io_context_))
  {
    io_thread_ = std::thread([this]() { io_context_.run(); });
  }

  ~IoContext()
  {
    io_context_.stop();
    if (io_thread_.joinable()) io_thread_.join();
  }

  boost::asio::io_context & get() { return io_context_; }

private:
  boost::asio::io_context io_context_;
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
  std::thread io_thread_;
};

namespace utils
{

/**
 * @brief Consistent Overhead Byte Stuffing(COBS)
 * @details Remove 0x00 from the data and represent the packet terminator as 0x00.
 */
class Cobs
{
public:
  static std::vector<uint8_t> encode(const std::vector<uint8_t> & input)
  {
    std::vector<uint8_t> output;
    output.reserve(input.size() + input.size() / 254 + 2);

    size_t code_idx = output.size();
    output.push_back(0);  // placeholder
    uint8_t code = 1;

    for (uint8_t byte : input) {
      if (byte != 0) {
        output.push_back(byte);
        code++;
      }
      if (byte == 0 || code == 0xFF) {
        output[code_idx] = code;
        code = 1;
        code_idx = output.size();
        if (byte == 0 || code == 0xFF) {
          output.push_back(0);
        }
      }
    }
    output[code_idx] = code;
    output.push_back(0x00);
    return output;
  }

  static std::vector<uint8_t> decode(const uint8_t * data, size_t length)
  {
    std::vector<uint8_t> output;
    output.reserve(length);
    size_t index = 0;

    while (index < length) {
      uint8_t code = data[index];
      index++;

      // If only terminal 0x00
      if (code == 0) break;

      for (uint8_t i = 1; i < code; i++) {
        if (index >= length) break;  // Error
        output.push_back(data[index++]);
      }
      if (code < 0xFF && index < length) {
        output.push_back(0);
      }
    }
    return output;
  }
};
}  // namespace utils
}  // namespace protolink

#endif  // !PROTOLINK__UTILS_HPP_
