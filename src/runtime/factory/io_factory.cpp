#include <factory_helpers.hpp>
#include <i2c_factory.hpp>
#include <io_factory.hpp>
#include <modbus_factory.hpp>
#include <mqtt_io_factory.hpp>

#include <stdexcept>
#include <variant>

namespace HomeAutomation {
namespace Runtime {

void IOFactory::createIOs(
    YAML::Node const &ioNode,
    std::shared_ptr<HomeAutomation::Runtime::TaskIOLogicComposite> ioLogic,
    std::shared_ptr<HomeAutomation::GV> gv) {
  if (!ioNode.IsDefined()) {
    return;
  }

  for (YAML::const_iterator systemIt = ioNode.begin(); systemIt != ioNode.end();
       ++systemIt) {
    auto const &ioEntry = *systemIt;

    auto const &ioType = Helper::getRequiredField<std::string>(ioEntry, "type");
    if (ioType == "i2c") {
      IOFactoryI2C::createIOs(ioEntry, ioLogic, gv);
    } else if (ioType == "mqtt") {
      MQTTIOFactory::createIOs(ioEntry, ioLogic, gv);
    } else if (ioType == "modbus-rtu") {
      ModbusRTUFactory::createIOs(ioEntry, ioLogic, gv);
    } else {
      throw std::invalid_argument("unsupported io type");
    }
  }
}

} // namespace Runtime
} // namespace HomeAutomation
