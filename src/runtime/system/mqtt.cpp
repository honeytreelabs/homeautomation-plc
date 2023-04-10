#include <mqtt.hpp>

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

void ClientPaho::connect() {
  try {
    client.connect(connOpts);

    recv_worker = std::thread(&ClientPaho::recvWorkerFun, this);
    send_worker = std::thread(&ClientPaho::sendWorkerFun, this);
  } catch (mqtt::exception const &exc) {
    spdlog::error("Could not connect to MQTT broker: {}", exc.what());
  }
}

void ClientPaho::send(mqtt::string_ref topic, mqtt::binary_ref payload,
                      int qos) {
  mqtt::message_ptr pubmsg =
      mqtt::make_message(std::move(topic), std::move(payload));
  pubmsg->set_qos(qos);
  send_msgs.put(pubmsg);
}

void ClientPaho::subscribe(std::string_view const &topic, int qos) {
  mqtt::string mqtt_topic{topic};
  topics.emplace_back(mqtt_topic);
  if (client.is_connected()) {
    client.subscribe(mqtt_topic, qos);
  }
}

mqtt::const_message_ptr ClientPaho::receive() {
  auto msg = recv_msgs.get();
  return msg.value_or(mqtt::const_message_ptr(nullptr));
}

void ClientPaho::reconnect() { client.reconnect(); }
bool ClientPaho::is_connected() const { return client.is_connected(); }

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
    if (!client.try_consume_message_for(&msg, 100ms)) {
      continue;
    }
    if (!client.is_connected()) {
      std::this_thread::sleep_for(100ms);
      continue;
    }
    recv_msgs.put(msg);
  }
}

void ClientPaho::sendWorkerFun() {
  while (!quit_cond) {
    auto pubmsg = send_msgs.get_for(100ms);
    if (pubmsg == std::nullopt) {
      continue;
    }
    try {
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
