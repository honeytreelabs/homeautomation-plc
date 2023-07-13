#pragma once

#include <mqtt.hpp>

#include <MQTTClient.h>
#include <spdlog/spdlog.h>

#include <optional>

namespace HomeAutomation {
namespace IO {
namespace MQTT {

class ClientPahoC : public RawClient {
public:
public:
  ClientPahoC(std::string const &address, std::string const &clientID) {
    int rc = MQTTClient_create(&client, address.c_str(), clientID.c_str(),
                               MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
      // throw exception
    }
  }

  ~ClientPahoC() { MQTTClient_destroy(&client); }

  bool connect() override {
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    return MQTTClient_connect(&client, &conn_opts) == MQTTCLIENT_SUCCESS;
  }

  bool is_connected() const override { return MQTTClient_isConnected(client); }

  void send(Topic &topic, Payload &payload, QoS qos) override {
    (void)topic;
    (void)payload;
    (void)qos;
  }

  bool subscribe(SubscribedTopic const &topic) override {
    (void)topic;
    return true;
  }

  OptionalMessage receive() override { return std::nullopt; }

  void disconnect() override {
    MQTTClient_disconnect(&client, 2 /* seconds */);
  }

private:
  MQTTClient client = nullptr;
};

} // namespace MQTT
} // namespace IO
} // namespace HomeAutomation
