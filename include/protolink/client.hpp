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
class Publisher
{
public:
  explicit Publisher(
    boost::asio::io_service & io_service, const std::string & ip_address, const uint16_t port,
    const uint16_t from_port, const rclcpp::Logger & logger = rclcpp::get_logger("protolink_udp"));
  const boost::asio::ip::udp::endpoint endpoint;
  const rclcpp::Logger logger;

  template <typename Proto>
  void send(const Proto & message)
  {
    std::string encoded_text = "";
    message.SerializeToString(&encoded_text);
    sendEncodedText(encoded_text);
  }

private:
  void sendEncodedText(const std::string & encoded_text);
  boost::asio::ip::udp::socket sock_;
};
}  // namespace udp_protocol

// namespace mqtt_protocol
// {
// const char * LWT_PAYLOAD = "Last will and testament.";

// class Publisher
// {
// public:
//   explicit Publisher(
//     const std::string & server_address, const std::string & client_id, const std::string & topic,
//     const int qos = 1, const rclcpp::Logger & logger = rclcpp::get_logger("protolink_mqtt"));
//   ~Publisher();

//   const std::string topic;
//   const int qos;
//   const rclcpp::Logger logger;

// private:
//   void sendEncodedText(const std::string & encoded_text);
//   mqtt::async_client client_impl_;
//   mqtt::callback callback_;
//   mqtt::token_ptr connect_token_;
//   std::thread connection_thread_;
//   bool connection_thread_running_;
// };

// class Subscriber
// {
//   class action_listener : public virtual mqtt::iaction_listener
//   {
//     std::string name_;

//     void on_failure(const mqtt::token & tok) override
//     {
//       std::cout << name_ << " failure";
//       if (tok.get_message_id() != 0)
//         std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
//       std::cout << std::endl;
//     }

//     void on_success(const mqtt::token & tok) override
//     {
//       std::cout << name_ << " success";
//       if (tok.get_message_id() != 0)
//         std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
//       auto top = tok.get_topics();
//       if (top && !top->empty())
//         std::cout << "\ttoken topic: '" << (*top)[0] << "', ..." << std::endl;
//       std::cout << std::endl;
//     }

//   public:
//     action_listener(const std::string & name) : name_(name) {}
//   };

//   class callback : public virtual mqtt::callback, public virtual mqtt::iaction_listener
//   {
//     // @todo update values
//     const std::string SERVER_ADDRESS = "mqtt://localhost:1883";
//     const std::string CLIENT_ID = "paho_cpp_async_subscribe";
//     const std::string TOPIC = "hello";

//     const int QOS = 1;
//     const int N_RETRY_ATTEMPTS = 5;

//     // Counter for the number of connection retries
//     int nretry_;
//     // The MQTT client
//     mqtt::async_client & cli_;
//     // Options to use if we need to reconnect
//     mqtt::connect_options & connOpts_;
//     // An action listener to display the result of actions.
//     action_listener subListener_;

//     // This deomonstrates manually reconnecting to the broker by calling
//     // connect() again. This is a possibility for an application that keeps
//     // a copy of it's original connect_options, or if the app wants to
//     // reconnect with different options.
//     // Another way this can be done manually, if using the same options, is
//     // to just call the async_client::reconnect() method.
//     void reconnect()
//     {
//       std::this_thread::sleep_for(std::chrono::milliseconds(2500));
//       try {
//         cli_.connect(connOpts_, nullptr, *this);
//       } catch (const mqtt::exception & exc) {
//         std::cerr << "Error: " << exc.what() << std::endl;
//         exit(1);
//       }
//     }

//     // Re-connection failure
//     void on_failure(const mqtt::token & tok) override
//     {
//       std::cout << "Connection attempt failed" << std::endl;
//       if (++nretry_ > N_RETRY_ATTEMPTS) exit(1);
//       reconnect();
//     }

//     // (Re)connection success
//     // Either this or connected() can be used for callbacks.
//     void on_success(const mqtt::token & tok) override {}

//     // (Re)connection success
//     void connected(const std::string & cause) override
//     {
//       std::cout << "\nConnection success" << std::endl;
//       std::cout << "\nSubscribing to topic '" << TOPIC << "'\n"
//                 << "\tfor client " << CLIENT_ID << " using QoS" << QOS << "\n"
//                 << "\nPress Q<Enter> to quit\n"
//                 << std::endl;

//       cli_.subscribe(TOPIC, QOS, nullptr, subListener_);
//     }

//     // Callback for when the connection is lost.
//     // This will initiate the attempt to manually reconnect.
//     void connection_lost(const std::string & cause) override
//     {
//       std::cout << "\nConnection lost" << std::endl;
//       if (!cause.empty()) std::cout << "\tcause: " << cause << std::endl;

//       std::cout << "Reconnecting..." << std::endl;
//       nretry_ = 0;
//       reconnect();
//     }

//     // Callback for when a message arrives.
//     void message_arrived(mqtt::const_message_ptr msg) override
//     {
//       std::cout << "Message arrived" << std::endl;
//       std::cout << "\ttopic: '" << msg->get_topic() << "'" << std::endl;
//       std::cout << "\tpayload: '" << msg->to_string() << "'\n" << std::endl;
//     }

//     void delivery_complete(mqtt::delivery_token_ptr token) override {}

//   public:
//     callback(mqtt::async_client & cli, mqtt::connect_options & connOpts)
//     : nretry_(0), cli_(cli), connOpts_(connOpts), subListener_("Subscription")
//     {
//     }
//   };

// public:
//   explicit Subscriber(
//     const std::string & server_address, const std::string & client_id, const std::string & topic,
//     const int qos = 1, const rclcpp::Logger & logger = rclcpp::get_logger("protolink_mqtt"));
//   ~Subscriber();

//   const std::string topic;
//   const int qos;
//   const rclcpp::Logger logger;

// private:
//   mqtt::async_client client_impl_;
// };
// }  // namespace mqtt_protocol
}  // namespace protolink
