#include <i2c_factory.hpp>
#include <io_factory.hpp>
#include <mqtt_io_factory.hpp>

#include <stdexcept>
#include <variant>

namespace HomeAutomation {
namespace Runtime {

void IOFactory::createIOs(YAML::Node const &ioNode,
                          std::shared_ptr<TaskIOLogicImpl> ioLogic,
                          HomeAutomation::GV *gv, MQTTClients *mqttClients) {
  if (!ioNode.IsDefined()) {
    return;
  }

  for (YAML::const_iterator systemIt = ioNode.begin(); systemIt != ioNode.end();
       ++systemIt) {
    auto const &ioEntry = *systemIt;

    auto const &ioType = ioEntry["type"].as<std::string>();
    if (ioType == "i2c") {
      IOFactoryI2C::createIOs(ioEntry, ioLogic, gv);
    } else if (ioType == "mqtt") {
      MQTTIOFactory::createIOs(ioEntry, ioLogic, gv, mqttClients);
    } else {
      throw std::invalid_argument("unsupported io type");
    }
  }
}

} // namespace Runtime
} // namespace HomeAutomation
