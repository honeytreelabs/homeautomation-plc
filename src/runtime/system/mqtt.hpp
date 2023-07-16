#pragma once

#include <circular_buffer.hpp>

#include <spdlog/spdlog.h>

#include <cstdint>

#include <atomic>
#include <functional>
#include <thread>
#include <vector>

namespace HomeAutomation {
namespace IO {
namespace MQTT {

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
  constexpr static QoS DEFAULT_QOS = 1;

  // defualt constructor required for the circular_buffer with pre-allocated
  // elements
  Message() = default;
  Message(Topic &&topic, Payload &&payload, QoS qos = DEFAULT_QOS)
      : topic_{std::move(topic)}, payload_{std::move(payload)}, qos_{qos} {}
  Message(Topic const &topic, Payload const &&payload, QoS qos = DEFAULT_QOS)
      : topic_{topic}, payload_{payload}, qos_{qos} {}

  inline Topic const &topic() const { return topic_; }
  inline Payload const &payload() const { return payload_; }
  inline std::string payload_str() const {
    return std::string{payload_.begin(), payload_.end()};
  }
  inline int qos() const { return qos_; }

private:
  Topic topic_;
  Payload payload_;
  QoS qos_;
};

using OptionalMessage = std::optional<Message>;
using Messages = HomeAutomation::circular_buffer<Message, 128>;

using SubscriptionCb = std::function<void(SubscribedTopic const &topic)>;

/**
 * @brief A synchronous client that actually sends the messsages in a blocking
 * fashion.
 *
 * Caveat: the client must handle synchronization internally;
 * multiple methods might be called simultaneously
 */
class RawClient {
public:
  virtual ~RawClient() = default;

  virtual bool connect() = 0;
  virtual bool is_connected() const = 0;
  virtual bool publish(Message const &message) = 0;
  virtual bool subscribe(SubscribedTopic const &topic) = 0;
  virtual OptionalMessage receive() = 0;
  virtual void set_on_resubscribe(SubscriptionCb cb) = 0;
  virtual void disconnect() = 0;
};

/**
 * @brief A buffering client that asynchronously sends given messages. It wraps
 * a synchronous client.
 */
class Client {
public:
  Client(std::unique_ptr<RawClient> client) : client_{std::move(client)} {}
  virtual ~Client() = default;

  virtual void connect() {
    running = true;
    reconnecter = std::thread{&Client::reconnect_logic, this};
  }
  virtual void publish(Message const &message) { client_->publish(message); }
  void publish(Topic const &topic, char const *payload) {
    publish(Message{topic, Payload{payload, payload + std::strlen(payload)}});
  }
  void publish(Topic const &topic, std::string const &payload) {
    publish(topic, payload.c_str());
  }
  virtual void subscribe(Topic const &topic, QoS qos) {
    client_->subscribe(SubscribedTopic{topic, qos});
  }
  void subscribe(Topic const &topic) { subscribe(topic, Message::DEFAULT_QOS); }
  virtual OptionalMessage receive() { return client_->receive(); }
  virtual void disconnect() {
    running = false;
    if (reconnecter.joinable()) {
      reconnecter.join();
    }
    if (client_->is_connected()) {
      client_->disconnect();
    }
  };
  virtual void set_on_resubscribe(SubscriptionCb cb) {
    client_->set_on_resubscribe(cb);
  }

private:
  void reconnect_logic() {
    using namespace std::chrono_literals;

    while (running) {
      if (!client_->is_connected()) {
        client_->connect();
      }
      std::this_thread::sleep_for(100ms);
    }
  }

  std::unique_ptr<RawClient> client_;
  std::atomic_bool running;
  std::thread reconnecter;
};

} // namespace MQTT
} // namespace IO
} // namespace HomeAutomation
