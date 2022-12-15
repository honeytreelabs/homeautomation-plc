#pragma once

#include <mqtt.hpp>
#include <runtime.hpp>

#include <yaml-cpp/yaml.h>

#include <map>

namespace HomeAutomation {
namespace Runtime {

class MQTTFactory {
public:
  static MQTTClients generateClients(YAML::Node const &mqttNode);

private:
  MQTTFactory() = delete;
};

} // namespace Runtime
} // namespace HomeAutomation
