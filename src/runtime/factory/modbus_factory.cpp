#include <factory_helpers.hpp>
#include <io_factory.hpp>
#include <modbus_dev.hpp>
#include <modbus_factory.hpp>
#include <task_io_bus.hpp>

#include <sstream>
#include <stdexcept>

namespace HomeAutomation {
namespace Runtime {

void ModbusRTUFactory::createIOs(
    YAML::Node const &ioNode,
    std::shared_ptr<HomeAutomation::Runtime::TaskIOLogicComposite> ioLogic,
    std::shared_ptr<HomeAutomation::GV> gv) {
  auto path = Helper::getRequiredField<std::string>(ioNode, "path");
  auto baud = Helper::getRequiredField<int>(ioNode, "baud");
  auto data_bit = Helper::getRequiredField<int>(ioNode, "data_bit");
  auto parity = Helper::getRequiredField<char>(ioNode, "parity");
  if (!(parity == 'N' || parity == 'E' || parity == 'O')) {
    throw std::invalid_argument("parity must be N, E, or O");
  }
  auto stop_bit = Helper::getRequiredField<int>(ioNode, "stop_bit");
  auto modbus = std::make_shared<HomeAutomation::IO::Modbus::BusRTU>(
      path, baud, parity, data_bit, stop_bit);

  CopySequenceInput inputSequence{};
  CopySequenceOutput outputSequence{};

  auto const &componentsNode = ioNode["components"];
  for (YAML::const_iterator componentsIt = componentsNode.begin();
       componentsIt != componentsNode.end(); ++componentsIt) {
    auto const &componentNode = *componentsIt;
    auto const &componentType =
        Helper::getRequiredField<std::string>(componentNode, "type");
    auto const slave = Helper::getRequiredField<int>(componentNode, "slave");
    if (componentType == "WP8026ADAM") {
      auto input =
          std::make_shared<HomeAutomation::IO::Modbus::WP8026ADAM>(slave);
      modbus->RegisterInput(input);
      Helper::insertCopySequenceBool(inputSequence, gv->inputs, input,
                                     componentNode["inputs"]);
    } else if (componentType == "R4S8CRMB") {
      auto output =
          std::make_shared<HomeAutomation::IO::Modbus::R4S8CRMB>(slave);
      modbus->RegisterOutput(output);
      Helper::insertCopySequenceBool(outputSequence, gv->outputs, output,
                                     componentNode["outputs"]);
    } else {
      std::stringstream s;
      s << "unknown component type " << componentType << " given";
      throw std::invalid_argument(s.str());
    }
  }

  auto modbusLogic = std::make_shared<BusIOLogic>(
      modbus, std::move(inputSequence), std::move(outputSequence));
  ioLogic->add(modbusLogic);
}

} // namespace Runtime
} // namespace HomeAutomation
