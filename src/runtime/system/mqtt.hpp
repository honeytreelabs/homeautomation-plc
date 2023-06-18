#pragma once

#include <circular_buffer.hpp>

#include <cstdint>
#include <mqtt/client.h>

#include <atomic>
#include <functional>
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
using Topic = std::string;
using Payload = std::vector<std::uint8_t>;
using QoS = int;

class Message {
public:
  Message(Topic &&topic, Payload &&payload, QoS qos)
      : topic_{std::move(topic)}, payload_{std::move(payload)}, qos_{qos} {}
  Message(Topic const &topic, Payload const &&payload, QoS qos)
      : topic_{topic}, payload_{payload}, qos_{qos} {}

  inline Topic const &topic() { return topic_; }
  inline Payload const &payload() { return payload_; }
  inline std::string payload_str() {
    return std::string{payload_.begin(), payload_.end()};
  }
  inline int qos() { return qos_; }

private:
  Topic topic_;
  Payload payload_;
  QoS qos_;
};

using OptionalMessage = std::optional<Message>;

class Client {
public:
  constexpr static QoS DEFAULT_QOS = 1;
  virtual ~Client() = default;

  virtual void connect() = 0;
  virtual void send(Topic &topic, Payload &payload, QoS qos) = 0;
  virtual void send(Topic &topic, Payload &payload) {
    send(topic, payload, DEFAULT_QOS);
  }
  virtual void send(Topic const &topic, char const *payload);
  virtual void send(Topic const &topic, std::string const &payload);
  virtual void subscribe(Topic const &topic, QoS qos) = 0;
  virtual void subscribe(Topic const &topic) { subscribe(topic, DEFAULT_QOS); }
  virtual OptionalMessage receive() = 0;
  virtual void disconnect() = 0;
  virtual void set_on_resubscribed(std::function<void()> cb) = 0;
};

class ClientPaho : public Client {
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

  ClientPaho(std::string const &address, std::string const &clientID,
             mqtt::connect_options connOpts);
  ClientPaho(std::string const &address, std::string const &clientID)
      : ClientPaho(address, clientID, getDefaultConnectOptions()) {}
  ClientPaho(std::string const &address)
      : ClientPaho(address, DFLT_CLIENT_ID) {}

  void connect() override;
  void send(Topic &topic, Payload &payload, QoS qos) override;
  void subscribe(Topic const &topic, QoS qos) override;
  OptionalMessage receive() override;
  void set_resubscribe();
  void set_on_resubscribed(std::function<void()> cb) override {
    on_resubscribed = cb;
  }
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
