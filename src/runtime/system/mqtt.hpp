#pragma once

#include <circular_buffer.hpp>

#include <mqtt/client.h>

#include <atomic>
#include <thread>
#include <vector>

namespace HomeAutomation {
namespace IO {
namespace MQTT {

using Messages = HomeAutomation::circular_buffer<mqtt::message_ptr, 128>;
using ConstMessages =
    HomeAutomation::circular_buffer<mqtt::const_message_ptr, 128>;
using Topics = std::vector<std::string>;

class ClientPaho {
public:
  static constexpr const char *DFLT_SERVER_ADDRESS{"tcp://localhost:1883"};
  static constexpr const char *DFLT_CLIENT_ID{"publish"};
  static constexpr const int DFLT_QOS{1};

  static mqtt::connect_options getDefaultConnectOptions() {
    using namespace std::chrono_literals;
    return mqtt::connect_options_builder()
        .keep_alive_interval(30s)
        .clean_session(true)
        .automatic_reconnect(500ms, 10s)
        .clean_session(false)
        .finalize();
  }

  ClientPaho(std::string const &address, std::string const &clientID,
             mqtt::connect_options connOpts)
      : client(address, clientID), conntok{}, send_msgs{}, recv_msgs{},
        quit_cond(false), topics{}, connOpts{connOpts}, send_worker{} {}
  ClientPaho(std::string const &address, std::string const &clientID)
      : ClientPaho(address, clientID, getDefaultConnectOptions()) {}
  ClientPaho(std::string const &address)
      : ClientPaho(address, DFLT_CLIENT_ID) {}
  ClientPaho() : ClientPaho(DFLT_SERVER_ADDRESS) {}
  virtual ~ClientPaho() {}

  void connect();
  void send(mqtt::string_ref topic, mqtt::binary_ref payload, int qos);
  inline void send(mqtt::string_ref topic, mqtt::binary_ref payload) {
    send(topic, payload, DFLT_QOS);
  }
  void subscribe(std::string_view const &topic, int qos);
  inline void subscribe(std::string_view const &topic) {
    subscribe(topic, DFLT_QOS);
  }
  mqtt::const_message_ptr receive();
  void reconnect();
  bool is_connected() const;
  void disconnect();

private:
  ClientPaho(ClientPaho const &);
  ClientPaho &operator=(ClientPaho const &);

  void recvWorkerFun();
  void sendWorkerFun();

  mqtt::client client;
  mqtt::token_ptr conntok;
  Messages send_msgs;
  ConstMessages recv_msgs;
  std::atomic_bool quit_cond;
  Topics topics;
  mqtt::connect_options connOpts;
  // Callback cb;
  std::thread recv_worker;
  std::thread send_worker;
};

} // namespace MQTT
} // namespace IO
} // namespace HomeAutomation
