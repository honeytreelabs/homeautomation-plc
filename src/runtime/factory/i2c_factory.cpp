#include <factory_helpers.hpp>
#include <i2c_bus.hpp>
#include <i2c_dev.hpp>
#include <i2c_factory.hpp>
#include <task_io_bus.hpp>

namespace HomeAutomation {

namespace Runtime {

void IOFactoryI2C::createIOs(
    YAML::Node const &ioNode,
    std::shared_ptr<HomeAutomation::Runtime::TaskIOLogicComposite> ioLogic,
    HomeAutomation::GV *gv) {
  auto i2cbus = std::make_shared<HomeAutomation::IO::I2C::RealBus>(
      ioNode["bus"].as<std::string>());

  CopySequenceInput inputSequence{};
  CopySequenceOutput outputSequence{};

  auto const &componentsNode = ioNode["components"];
  for (YAML::const_iterator componentsIt = componentsNode.begin();
       componentsIt != componentsNode.end(); ++componentsIt) {
    auto const address =
        std::stoul(componentsIt->first.as<std::string>(), nullptr, 16);
    auto const &componentNode = componentsIt->second;

    if (componentNode["direction"].as<std::string>() == "input") {
      std::shared_ptr<HomeAutomation::IO::I2C::DigitalInputModule> input;
      if (componentNode["type"].as<std::string>() == "pcf8574") {
        input =
            std::make_shared<HomeAutomation::IO::I2C::PCF8574Input>(address);
      } else {
        throw std::invalid_argument("unknown i2c component type");
      }
      i2cbus->RegisterInput(input);
      Helper::insertCopySequenceBool(inputSequence, gv->inputs, input,
                                     componentNode["inputs"]);
      // TODO insertCopySequenceInput
    } else if (componentNode["direction"].as<std::string>() == "output") {
      std::shared_ptr<HomeAutomation::IO::I2C::DigitalOutputModule> output;
      if (componentNode["type"].as<std::string>() == "pcf8574") {
        output =
            std::make_shared<HomeAutomation::IO::I2C::PCF8574Output>(address);
      } else if (componentNode["type"].as<std::string>() == "max7311") {
        output =
            std::make_shared<HomeAutomation::IO::I2C::MAX7311Output>(address);
      } else {
        throw std::invalid_argument("unknown i2c component type");
      }
      i2cbus->RegisterOutput(output);
      Helper::insertCopySequenceBool(outputSequence, gv->outputs, output,
                                     componentNode["outputs"]);
    } else {
      throw std::invalid_argument("unknown i2c component direction");
    }
  }

  auto i2cLogic = std::make_shared<BusIOLogic>(i2cbus, std::move(inputSequence),
                                               std::move(outputSequence));
  ioLogic->add(i2cLogic);
}

} // namespace Runtime
} // namespace HomeAutomation
