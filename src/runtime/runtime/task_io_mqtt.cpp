#include <task_io_mqtt.hpp>

namespace HomeAutomation {
namespace Runtime {

constexpr char const *PAYLOAD_TRUE = "1";
constexpr char const *PAYLOAD_FALSE = "0";

void MQTTIOLogic::init() {
  mqttClient->connect();
  for (auto const &[topic, varName] : inputs) {
    mqttClient->subscribe(topic);
  }
  for (auto const &[varName, topic] : outputs) {
    outputValues[varName] = false;
  }
}

void MQTTIOLogic::shutdown() { mqttClient->disconnect(); }

void MQTTIOLogic::before() {
  for (auto const &[topic, varName] : inputs) {
    gv->inputs[varName] = false;
  }

  while (auto msg = mqttClient->receive()) {
    auto const inputIt = inputs.find(msg.value().topic());
    if (inputIt == inputs.end()) {
      continue;
    }
    auto const &varName = inputIt->second;
    if (msg.value().payload_str() == PAYLOAD_TRUE) {
      gv->inputs[varName] = true;
    } else if (msg.value().payload_str() == PAYLOAD_FALSE) {
      gv->inputs[varName] = false;
    }
  }
}

void MQTTIOLogic::after() {
  // send all changed variable values
  for (auto const &[varName, topic] : outputs) {
    // only send outputValue if R_TRIG triggers
    // this behaviour should be configurable in the future
    if (!std::get<bool>(outputValues[varName]) &&
        std::get<bool>(gv->outputs[varName])) {
      // send true
      mqttClient->send(topic, PAYLOAD_TRUE);
    }
    outputValues[varName] = gv->outputs[varName];
  }
}

} // namespace Runtime
} // namespace HomeAutomation
