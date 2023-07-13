#pragma once

#include <mqtt.hpp>

#include <mqtt/client.h>

namespace HomeAutomation {
namespace IO {
namespace MQTT {

using Messages = HomeAutomation::circular_buffer<mqtt::message_ptr, 128>;
using ConstMessages =
    HomeAutomation::circular_buffer<mqtt::const_message_ptr, 128>;

class ClientPahoPP : public RawClient {
public:
  static constexpr const char *DFLT_CLIENT_ID{"publish"};
  static constexpr const int DFLT_QOS{1};

  static mqtt::connect_options getDefaultConnectOptions() {
    using namespace std::chrono_literals;
    return mqtt::connect_options_builder()
        .keep_alive_interval(2s)
        .clean_session(true)
        .automatic_reconnect(500ms, 2s)
        .finalize();
  }

  ClientPahoPP(std::string const &address, std::string const &clientID,
               mqtt::connect_options connOpts);
  ClientPahoPP(std::string const &address, std::string const &clientID)
      : ClientPahoPP(address, clientID, getDefaultConnectOptions()) {}
  ClientPahoPP(std::string const &address)
      : ClientPahoPP(address, DFLT_CLIENT_ID) {}

  bool connect() override;
  void send(Topic &topic, Payload &payload, QoS qos) override;
  bool subscribe(SubscribedTopic const &topic) override;
  OptionalMessage receive() override;
  void set_resubscribe();
  void disconnect() override;

private:
  void recvWorkerFun();
  void sendWorkerFun();
  void resubscribe();

  mqtt::client client;
  std::shared_ptr<mqtt::callback> cb;
  std::atomic_bool must_resubscribe;
  Messages send_msgs;
  ConstMessages recv_msgs;
  std::atomic_bool quit_cond;
  Topics topics;
  mqtt::connect_options connOpts;
  // Callback cb;
  std::thread recv_worker;
  std::thread send_worker;
  std::function<void()> on_resubscribed;
};

} // namespace MQTT
} // namespace IO
} // namespace HomeAutomation
