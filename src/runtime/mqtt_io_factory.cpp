#include <io_impl.hpp>
#include <mqtt_impl.hpp>
#include <mqtt_io_factory.hpp>

#include <exception>

namespace HomeAutomation {
namespace Runtime {

void MQTTIOFactory::createIOs(YAML::Node const &ioNode,
                              std::shared_ptr<TaskIOLogicImpl> ioLogic,
                              HomeAutomation::GV *gv,
                              MQTTClients *mqttClients) {
  auto const &clientNode = ioNode["client"];
  if (!clientNode.IsDefined()) {
    throw std::invalid_argument("required mqtt client id not provided");
  }
  auto const mqttClientIt = mqttClients->find(clientNode.as<std::string>());
  if (mqttClientIt == mqttClients->end()) {
    throw std::invalid_argument("specified mqtt client not configured");
  }
  auto &mqttClient = mqttClientIt->second;

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
      std::move(inputMapping), std::move(outputMapping), gv, &mqttClient);
  ioLogic->add(mqttLogic);
}

} // namespace Runtime
} // namespace HomeAutomation
