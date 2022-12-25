#include <mqtt_impl.hpp>

namespace HomeAutomation {
namespace Runtime {

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
    auto const inputIt = inputs.find(msg->get_topic());
    if (inputIt == inputs.end()) {
      continue;
    }
    auto const &varName = inputIt->second;
    if (msg->get_payload_str() == "1") {
      gv->inputs[varName] = true;
    } else if (msg->get_payload_str() == "0") {
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
      mqttClient->send(topic, "1");
    }
    outputValues[varName] = gv->outputs[varName];
  }
}

} // namespace Runtime
} // namespace HomeAutomation
