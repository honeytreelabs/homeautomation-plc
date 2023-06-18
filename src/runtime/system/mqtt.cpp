#include <mqtt.hpp>

#include <optional>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

using namespace std::chrono_literals;

namespace HomeAutomation {
namespace IO {
namespace MQTT {

void Client::send(Topic const &topic, char const *payload) {
  auto length = std::strlen(payload);
  Payload payload_fwd;
  payload_fwd.reserve(length);
  for (std::size_t i = 0; i < length; i++) {
    payload_fwd.push_back(static_cast<std::uint8_t>(payload[i]));
  }
  std::string topic_fwd{topic};
  send(topic_fwd, payload_fwd);
}
void Client::send(Topic const &topic, std::string const &payload) {
  send(topic, payload.c_str());
}

class ClientCb final : public mqtt::callback {
public:
  ClientCb(ClientPaho &client) : pahoClient{client} {}
  void connected(const std::string &cause) override {
    spdlog::info("Connected: {}", cause);
    if (cause == "automatic reconnect") {
      pahoClient.set_resubscribe();
    }
  }
  void connection_lost(const std::string &cause) override {
    spdlog::info("Connection lost: {}",
                 cause != "" ? cause : "no reason given");
  }

private:
  ClientPaho &pahoClient;
};

ClientPaho::ClientPaho(std::string const &address, std::string const &clientID,
                       mqtt::connect_options connOpts)
    : client(address, clientID), cb{std::make_shared<ClientCb>(*this)},
      must_resubscribe{false}, send_msgs{}, recv_msgs{},
      quit_cond(false), topics{}, connOpts{connOpts}, send_worker{},
      on_resubscribed{[]() {}} {
  client.set_callback(*cb);
}

void ClientPaho::connect() {
  try {
    client.connect(connOpts);

    recv_worker = std::thread(&ClientPaho::recvWorkerFun, this);
    send_worker = std::thread(&ClientPaho::sendWorkerFun, this);
  } catch (mqtt::exception const &exc) {
    spdlog::error("Could not connect to MQTT broker: {}", exc.what());
  }
}

void ClientPaho::send(std::string &topic, Payload &payload, QoS qos) {
  mqtt::message_ptr pubmsg = mqtt::make_message(
      std::move(topic), std::string(payload.begin(), payload.end()));
  pubmsg->set_qos(qos == Client::DEFAULT_QOS ? DFLT_QOS : qos);
  send_msgs.put(pubmsg);
}

void ClientPaho::subscribe(std::string const &topic, QoS qos) {
  topics.emplace_back(SubscribedTopic{topic, qos});
  if (client.is_connected()) {
    client.subscribe(topic, qos);
  }
}

void ClientPaho::resubscribe() {
  for (auto const &subscribedTopic : topics) {
    spdlog::info("Resubscribing to topic \"{}\" with QoS {}",
                 subscribedTopic.topic, subscribedTopic.qos);
    client.subscribe(subscribedTopic.topic, subscribedTopic.qos);
  }
  must_resubscribe = false;
}

OptionalMessage ClientPaho::receive() {
  auto optional_msg = recv_msgs.get();
  if (!optional_msg) {
    return std::nullopt;
  }
  auto const &msg = optional_msg.value();
  Payload data{};
  for (char c : msg->get_payload()) {
    data.push_back(static_cast<std::uint8_t>(c));
  }
  return Message{msg->get_topic(), std::move(data), msg->get_qos()};
}

void ClientPaho::set_resubscribe() { must_resubscribe = true; }

void ClientPaho::disconnect() {
  quit_cond = true;
  if (send_worker.joinable()) {
    send_worker.join();
  }
  if (recv_worker.joinable()) {
    recv_worker.join();
  }

  try {
    client.disconnect();
  } catch (mqtt::exception const &exc) {
    spdlog::error("could not disconnect mqtt client: {}", exc.what());
  }
}

void ClientPaho::recvWorkerFun() {
  while (!quit_cond) {
    mqtt::const_message_ptr msg;
    if (client.try_consume_message_for(&msg, 100ms) && client.is_connected()) {
      spdlog::info("Received message with payload \"{}\" on topic \"{}\"",
                   msg->get_payload(), msg->get_topic());
      recv_msgs.put(msg);
    }
    if (must_resubscribe) {
      resubscribe();
      on_resubscribed();
    }
  }
}

void ClientPaho::sendWorkerFun() {
  while (!quit_cond) {
    auto pubmsg = send_msgs.get_for(100ms);
    if (pubmsg == std::nullopt) {
      continue;
    }
    try {
      spdlog::info("Publishing message with payload \"{}\" on topic \"{}\"",
                   pubmsg.value()->get_payload(), pubmsg.value()->get_topic());
      client.publish(pubmsg.value());
    } catch (mqtt::exception &exc) {
      spdlog::error("Error publishing message: {}", exc.what());
      std::this_thread::sleep_for(100ms);
    }
  }
}

} // namespace MQTT
} // namespace IO
} // namespace HomeAutomation
