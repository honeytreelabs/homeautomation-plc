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
using Messages = HomeAutomation::circular_buffer<Message, 128>;

using SubscriptionCb = std::function<void()>;

/**
 * @brief A synchronous client that actually sends the messsages in a blocking
 * fashion.
 */
class RawClient {
public:
  virtual ~RawClient() = default;

  virtual bool connect() = 0;
  virtual bool is_connected() const = 0;
  virtual void send(Message const &message) = 0;
  virtual bool subscribe(SubscribedTopic const &topic) = 0;
  virtual OptionalMessage receive(std::chrono::milliseconds timeout) = 0;
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
    reconnecter = std::thread(&Client::reconnect_logic, this);
  }
  virtual void send(Message const &message) { client_->send(message); }
  void send(Topic const &topic, char const *payload);
  virtual void send(Topic const &topic, std::string const &payload);
  virtual void subscribe(Topic const &topic, QoS qos) {
    topics.emplace_back(topic, qos);
    client_->subscribe(SubscribedTopic{topic, qos});
  }
  void subscribe(Topic const &topic) { subscribe(topic, Message::DEFAULT_QOS); }
  virtual OptionalMessage receive() { return {}; }
  virtual void disconnect() {
    running = false;
    if (reconnecter.joinable()) {
      reconnecter.join();
    }
    topics.clear();
    if (client_->is_connected()) {
      client_->disconnect();
    }
  };
  virtual void set_on_resubscribed(SubscriptionCb cb) { cb_ = cb; }

private:
  void reconnect_logic() {
    using namespace std::chrono_literals;

    while (running) {
      if (!client_->is_connected()) {
        if (client_->connect()) {
          std::for_each(
              topics.begin(), topics.end(),
              [this](auto const &topic) { client_->subscribe(topic); });
        } else {
          spdlog::warn("Could not connect to configured MQTT broker.");
        }
      }

      std::this_thread::sleep_for(1s);
    }
  }

  void send_worker_logic() {
    using namespace std::chrono_literals;
    while (running) {
      auto pubmsg = send_msgs.get_for(100ms);
      if (!pubmsg) {
        continue;
      }
      spdlog::info("Publishing message with payload \"{}\" on topic \"{}\"",
                   pubmsg.value().payload_str(), pubmsg.value().topic());
      client_->send(pubmsg.value());
    }
  }

  void recv_worker_logic() {}

  std::unique_ptr<RawClient> client_;
  std::atomic_bool running = false;
  std::thread reconnecter;
  Topics topics;
  SubscriptionCb cb_;
  Messages send_msgs;
  Messages recv_msgs;
};

} // namespace MQTT
} // namespace IO
} // namespace HomeAutomation
