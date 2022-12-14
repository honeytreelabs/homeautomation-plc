#include <mqtt.hpp>

#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace HomeAutomation {
namespace IO {
namespace MQTT {

void Callback::connected(const std::string & /*cause*/) {
  for (auto &topic : topics) {
    client.subscribe(topic, 1 /* qos */);
  }
}

void Callback::reconnect() {
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2500ms); // TODO make interruptable
  client.connect(connOpts, nullptr, *this);
}

void Callback::connection_lost(const std::string &cause) {
  std::cout << "\nConnection lost" << std::endl;
  if (!cause.empty()) {
    std::cout << "\tcause: " << cause << std::endl;
  }
  reconnect();
}

void Callback::message_arrived(mqtt::const_message_ptr msg) {
  messages.put(msg);
}

void Callback::delivery_complete(mqtt::delivery_token_ptr tok) {
  std::cout << "\tDelivery complete for token: "
            << (tok ? tok->get_message_id() : -1) << std::endl;
}

/**
 * A base action listener.
 */
class sender_action_listener : public virtual mqtt::iaction_listener {
protected:
  void on_failure(const mqtt::token &tok) override {
    std::cout << "\tListener failure for token: " << tok.get_message_id()
              << std::endl;
  }

  void on_success(const mqtt::token &tok) override {
    std::cout << "\tListener success for token: " << tok.get_message_id()
              << std::endl;
  }
};

/**
 * A derived action listener for publish events.
 */
class delivery_action_listener : public sender_action_listener {
  std::atomic<bool> done_;

  void on_failure(const mqtt::token &tok) override {
    sender_action_listener::on_failure(tok);
    done_ = true;
  }

  void on_success(const mqtt::token &tok) override {
    sender_action_listener::on_success(tok);
    done_ = true;
  }

public:
  delivery_action_listener() : done_(false) {}
  bool is_done() const { return done_; }
};

void ClientPaho::connect() {
  // client should be resilient to MQTT broker outages (e.g. retry)
  // but this is not implemented yet
  try {
    conntok = client.connect(connOpts);
    conntok->wait();

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
  return msg.value_or(mqtt::const_message_ptr(0));
}

void ClientPaho::disconnect() {
  quit_cond = true;
  if (send_worker.joinable()) {
    send_worker.join();
  }

  // Double check that there are no pending tokens
  auto toks = client.get_pending_delivery_tokens();
  if (!toks.empty()) {
    std::cout << "Error: There are pending delivery tokens!" << std::endl;
  }

  // Disconnect
  try {
    conntok = client.disconnect();
    conntok->wait();
  } catch (mqtt::exception const &exc) {
    spdlog::error("could not disconnect mqtt client: {}", exc.what());
  }
}

void ClientPaho::sendWorkerFun() {
  using namespace std::chrono_literals;
  while (!quit_cond) {
    //   wait for queue with timeout
    auto pubmsg = send_msgs.get_for(100ms);
    if (pubmsg == std::nullopt) {
      continue;
    }
    /* hand elem over to MQTT stack */
    try {
      client.publish(pubmsg.value())->wait_for(1s);
    } catch (mqtt::exception &exc) {
      // TODO log error
    }
  }
}

} // namespace MQTT
} // namespace IO
} // namespace HomeAutomation
