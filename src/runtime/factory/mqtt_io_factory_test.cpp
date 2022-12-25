#include <factory_helpers.hpp>
#include <gv_factory.hpp>
#include <mqtt_io_factory.hpp>

#include <stdexcept>
#include <yaml-cpp/yaml.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("mqtt io factory: initialize mqtt with all required arguments",
          "[single-file]") {
  HomeAutomation::GV gv;

  auto const &rootNode = YAML::Load(R"(---
io:
  - type: mqtt
    client:
      address: tcp://localhost:1883
      client_id: foo::bar
    inputs: {}
    outputs: {}
)");

  auto taskIoLogicImpl =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
  REQUIRE_NOTHROW(HomeAutomation::Runtime::MQTTIOFactory::createIOs(
      rootNode["io"][0], taskIoLogicImpl, &gv));
}
