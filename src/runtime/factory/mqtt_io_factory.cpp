#include <factory_helpers.hpp>
#include <mqtt_io_factory.hpp>
#include <task_io_bus.hpp>
#include <task_io_mqtt.hpp>

#include <exception>
#include <stdexcept>

namespace HomeAutomation {
namespace Runtime {

static std::shared_ptr<HomeAutomation::Components::MQTT::ClientPaho>
generateClient(YAML::Node const &clientNode) {
  auto mqtt_options = Components::MQTT::ClientPaho::getDefaultConnectOptions();
  auto const &nodeUsername = clientNode["username"];
  auto const &nodePassword = clientNode["password"];
  if (nodeUsername.IsDefined() && nodePassword.IsDefined()) {
    mqtt_options.set_user_name(clientNode["username"].as<std::string>());
    mqtt_options.set_password(clientNode["password"].as<std::string>());
  }
  auto address = Helper::getRequiredField<std::string>(clientNode, "address");
  auto client_id =
      Helper::getRequiredField<std::string>(clientNode, "client_id");
  return std::make_shared<HomeAutomation::Components::MQTT::ClientPaho>(
      address, client_id, mqtt_options);
}

void MQTTIOFactory::createIOs(YAML::Node const &ioNode,
                              std::shared_ptr<TaskIOLogicImpl> ioLogic,
                              HomeAutomation::GV *gv) {
  auto const &mqttClientNode = ioNode["client"];
  if (!mqttClientNode.IsDefined()) {
    throw std::invalid_argument("required client field not defined");
  }
  auto mqttClient = generateClient(mqttClientNode);

  InputMapping inputMapping{};
  auto const &inputsNode = ioNode["inputs"];
  for (YAML::const_iterator inputsIt = inputsNode.begin();
       inputsIt != inputsNode.end(); ++inputsIt) {
    auto topic = inputsIt->first.as<std::string>();
    auto gvName = inputsIt->second.as<std::string>();
    gv->inputs[gvName] = false;
    inputMapping.emplace(std::move(topic), std::move(gvName));
  }

  OutputMapping outputMapping{};
  auto const &outputsNode = ioNode["outputs"];
  for (YAML::const_iterator outputsIt = outputsNode.begin();
       outputsIt != outputsNode.end(); ++outputsIt) {
    auto topic = outputsIt->first.as<std::string>();
    auto gvName = outputsIt->second.as<std::string>();
    gv->outputs[gvName] = false;
    outputMapping.emplace(std::move(gvName), std::move(topic));
  }

  auto mqttLogic = std::make_shared<MQTTIOLogic>(
      std::move(inputMapping), std::move(outputMapping), gv, mqttClient);
  ioLogic->add(mqttLogic);
}

} // namespace Runtime
} // namespace HomeAutomation
