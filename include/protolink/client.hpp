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

#include <boost/asio.hpp>

namespace protolink
{
namespace udp
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
}  // namespace udp
}  // namespace protolink
