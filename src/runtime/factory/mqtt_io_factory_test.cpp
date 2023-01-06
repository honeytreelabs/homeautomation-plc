#include <factory_helpers.hpp>
#include <gv_factory.hpp>
#include <mqtt_io_factory.hpp>

#include <stdexcept>
#include <yaml-cpp/yaml.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

TEST_CASE("mqtt io factory: initialize mqtt with all required arguments") {
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
