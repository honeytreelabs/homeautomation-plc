#include "scheduler_impl.hpp"
#include <gv_factory.hpp>
#include <io_factory.hpp>

#include <stdexcept>
#include <yaml-cpp/yaml.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("initialize non-existing global var", "[single-file]") {
  HomeAutomation::GV gv;

  auto const &rootNode = YAML::Load(R"(---
global_vars:
  inputs:
    erroneous:
      init_val: true
io:
  - type: i2c
    bus: /dev/i2c-1
    components:
      3b:  # i2c address
        type: pcf8574
        direction: input
        inputs:
          0: io_that_does_not_exist_yet
)");

  auto taskIoLogicImpl =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicImpl>();
  REQUIRE_NOTHROW(HomeAutomation::Runtime::IOFactory::createIOs(
      rootNode["io"], taskIoLogicImpl, &gv));
  REQUIRE_THROWS_AS(HomeAutomation::Runtime::GVFactory::initializeGVs(
                        rootNode["global_vars"], &gv),
                    std::invalid_argument);
}
