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
namespace udp
{
Client::Client(const std::string & ip_address, const uint16_t port)
: endpoint(boost::asio::ip::udp::endpoint(
    boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(ip_address), port))),
  sock_(io_service_, endpoint)
{
}

void Client::sendEncodedText(const std::string & encoded_text)
{
  sock_.send_to(boost::asio::buffer(encoded_text), endpoint);
}
}  // namespace udp
}  // namespace protolink
