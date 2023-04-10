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
struct SubscribedTopic {
  std::string const topic;
  int qos;
};
using Topics = std::vector<SubscribedTopic>;

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
        .finalize();
  }

  ClientPaho(std::string const &address, std::string const &clientID,
             mqtt::connect_options connOpts);
  ClientPaho(std::string const &address, std::string const &clientID)
      : ClientPaho(address, clientID, getDefaultConnectOptions()) {}
  virtual ~ClientPaho() {}

  void connect();
  void send(mqtt::string_ref topic, mqtt::binary_ref payload, int qos);
  inline void send(mqtt::string_ref topic, mqtt::binary_ref payload) {
    send(topic, payload, DFLT_QOS);
  }
  void subscribe(std::string const &topic, int qos);
  inline void subscribe(std::string const &topic) {
    subscribe(topic, DFLT_QOS);
  }
  mqtt::const_message_ptr receive();
  void set_resubscribe();
  bool is_connected() const;
  void disconnect();

private:
  ClientPaho(std::string const &address)
      : ClientPaho(address, DFLT_CLIENT_ID) {}
  ClientPaho() : ClientPaho(DFLT_SERVER_ADDRESS) {}
  ClientPaho(ClientPaho const &);
  ClientPaho &operator=(ClientPaho const &);

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
};

} // namespace MQTT
} // namespace IO
} // namespace HomeAutomation
