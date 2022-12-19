#include <mqtt_factory.hpp>

namespace HomeAutomation {
namespace Runtime {

std::shared_ptr<MQTTClients>
MQTTFactory::generateClients(YAML::Node const &mqttNode) {
  auto clients = std::make_shared<MQTTClients>();

  if (!mqttNode.IsDefined()) {
    return clients;
  }

  for (YAML::const_iterator it = mqttNode.begin(); it != mqttNode.end(); ++it) {
    auto const &mqtt = it->second;
    auto mqtt_options =
        Components::MQTT::ClientPaho::getDefaultConnectOptions();
    auto const &nodeUsername = mqtt["username"];
    auto const &nodePassword = mqtt["password"];
    if (nodeUsername.IsDefined() && nodePassword.IsDefined()) {
      mqtt_options.set_user_name(mqtt["username"].as<std::string>());
      mqtt_options.set_password(mqtt["password"].as<std::string>());
    }
    auto inserted = clients->emplace(
        std::piecewise_construct,
        std::forward_as_tuple(it->first.as<std::string>()),
        std::forward_as_tuple(mqtt["address"].as<std::string>(),
                              mqtt["client_id"].as<std::string>(),
                              mqtt_options));
    auto const &isInserted = inserted.second;
    auto &mqttClient = inserted.first->second;

    if (isInserted) {
      auto const &topicsNode = mqtt["topics"];
      if (topicsNode.IsDefined()) {
        for (YAML::const_iterator it = topicsNode.begin();
             it != topicsNode.end(); ++it) {
          mqttClient.subscribe(it->as<std::string>());
        }
      }
    }
  }

  return clients;
}

} // namespace Runtime
} // namespace HomeAutomation
