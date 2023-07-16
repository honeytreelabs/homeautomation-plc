#pragma once

#include <mqtt.hpp>

#include <MQTTClient.h>
#include <spdlog/spdlog.h>

#include <optional>
#include <stdexcept>

namespace HomeAutomation {
namespace IO {
namespace MQTT {

class ClientPahoC : public RawClient {
public:
public:
  ClientPahoC(std::string const &address, std::string const &clientID) {
    int rc = MQTTClient_create(&client, address.c_str(), clientID.c_str(),
                               MQTTCLIENT_PERSISTENCE_NONE, nullptr);
    if (rc != MQTTCLIENT_SUCCESS) {
      throw std::runtime_error("Could not create MQTT client.");
    }
  }

  ~ClientPahoC() { MQTTClient_destroy(&client); }

  bool connect() override {
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    return MQTTClient_connect(client, &conn_opts) == MQTTCLIENT_SUCCESS;
  }

  bool is_connected() const override { return MQTTClient_isConnected(client); }

  bool publish(Message const &message,
               std::chrono::milliseconds const &timeout) override {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    // this might be unsafe; so maybe changing it
    pubmsg.payload = const_cast<void *>(
        static_cast<void const *>(message.payload_str().c_str()));
    pubmsg.payloadlen = message.payload().size();
    pubmsg.qos = message.qos();
    pubmsg.retained = 0;
    MQTTClient_deliveryToken token;
    spdlog::info("Publishing message \"{}\" on topic \"{}\"",
                 message.payload_str(), message.topic());
    auto rc = MQTTClient_publishMessage(client, message.topic().c_str(),
                                        &pubmsg, &token);
    if (rc != MQTTCLIENT_SUCCESS) {
      spdlog::error("Could not publish MQTT message with error {}",
                    MQTTClient_strerror(rc));
      return false;
    }

    rc = MQTTClient_waitForCompletion(client, token, timeout.count());
    if (rc != MQTTCLIENT_SUCCESS) {
      spdlog::error(
          "Could not wait for completion of delivery token with error {}",
          MQTTClient_strerror(rc));
      return false;
    }
    return true;
  }

  bool subscribe(SubscribedTopic const &topic) override {
    spdlog::info("Subscribing to topic {}", topic.topic);
    auto rc = MQTTClient_subscribe(client, topic.topic.c_str(), topic.qos);
    if (rc != MQTTCLIENT_SUCCESS) {
      spdlog::error("Could not subscribe to topic {}; error: {}", topic.topic,
                    MQTTClient_strerror(rc));
      return false;
    }
    spdlog::info("Successfully subscribed to topic {}", topic.topic);
    return true;
  }

  OptionalMessage receive(std::chrono::milliseconds const &timeout) override {
    char *topicName;
    int topicLen;
    MQTTClient_message *message;
    auto rc = MQTTClient_receive(client, &topicName, &topicLen, &message,
                                 timeout.count());
    if (rc != MQTTCLIENT_SUCCESS) {
      spdlog::error("Could not receive MQTT message; error: {}",
                    MQTTClient_strerror(rc));
      return {};
    }
    if (!message) {
      return {};
    }

    spdlog::info("Some message has arrived.");
    auto result = Message{
        topicName,
        Payload{static_cast<char *>(message->payload),
                static_cast<char *>(message->payload) + message->payloadlen},
        Message::DEFAULT_QOS};
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return result;
  }

  void disconnect() override { MQTTClient_disconnect(client, 2 /* seconds */); }

private:
  MQTTClient client = nullptr;
};

} // namespace MQTT
} // namespace IO
} // namespace HomeAutomation
