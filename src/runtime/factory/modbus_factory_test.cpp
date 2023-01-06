#include <factory_helpers.hpp>
#include <gv_factory.hpp>
#include <modbus_factory.hpp>

#include <stdexcept>
#include <yaml-cpp/yaml.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

TEST_CASE("modbus factory: initialize modbus with all required arguments") {
  HomeAutomation::GV gv;

  auto const &rootNode = YAML::Load(R"(---
io:
  - type: modbus
    path: /dev/ttyUSB0
    baud: 9600
    data_bit: 8
    parity: N
    stop_bit: 1
    components: []
)");

  auto taskIoLogicImpl =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
  REQUIRE_NOTHROW(HomeAutomation::Runtime::ModbusRTUFactory::createIOs(
      rootNode["io"][0], taskIoLogicImpl, &gv));
}

TEST_CASE("modbus factory: initialize modbus with missing required argument") {
  HomeAutomation::GV gv;

  auto const &rootNode = YAML::Load(R"(---
io:
  - type: modbus
    # path missing
    baud: 9600
    data_bit: 8
    parity: N
    stop_bit: 1
    components: []
)");

  auto taskIoLogicImpl =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
  REQUIRE_THROWS_AS(HomeAutomation::Runtime::ModbusRTUFactory::createIOs(
                        rootNode["io"][0], taskIoLogicImpl, &gv),
                    std::invalid_argument);
}

TEST_CASE("modbus factory: initialize modbus with invalid parity setting") {
  HomeAutomation::GV gv;

  auto const &rootNode = YAML::Load(R"(---
io:
  - type: modbus
    path: /dev/ttyUSB0
    baud: 9600
    data_bit: 8
    parity: I  # invalid setting
    stop_bit: 1
    components: []
)");

  auto taskIoLogicImpl =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
  REQUIRE_THROWS_AS(HomeAutomation::Runtime::ModbusRTUFactory::createIOs(
                        rootNode["io"][0], taskIoLogicImpl, &gv),
                    std::invalid_argument);
}

TEST_CASE("modbus factory: initialize modbus with all required arguments and "
          "components") {
  HomeAutomation::GV gv;

  auto const &rootNode = YAML::Load(R"(---
io:
  - type: modbus
    path: /dev/ttyUSB0
    baud: 9600
    data_bit: 8
    parity: N
    stop_bit: 1
    components:
      - type: WP8026ADAM
        slave: 01
        inputs:
          0: one
          1: two
          2: three
      - type: R4S8CRMB
        slave: 01
        outputs:
          0: one
          1: two
          2: three
)");

  auto taskIoLogicImpl =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
  REQUIRE_NOTHROW(HomeAutomation::Runtime::ModbusRTUFactory::createIOs(
      rootNode["io"][0], taskIoLogicImpl, &gv));
}

TEST_CASE("modbus factory: check parity setting") {
  HomeAutomation::GV gv;

  auto const &rootNode = YAML::Load(R"(---
io:
  - parity: N
)");

  REQUIRE_NOTHROW(HomeAutomation::Runtime::Helper::getRequiredField<char>(
      rootNode["io"][0], "parity"));
  REQUIRE(HomeAutomation::Runtime::Helper::getRequiredField<char>(
              rootNode["io"][0], "parity") == 'N');
}
