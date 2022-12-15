#include <gv_factory.hpp>
#include <io_factory.hpp>

#include <stdexcept>
#include <yaml-cpp/yaml.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("reference non-existing global var", "[single-file]") {
  auto gv = HomeAutomation::Runtime::GVFactory::generateGVs(YAML::Load(R"(---
inputs:
  one:
    type: bool
    init_val: false
)"));

  auto const &ioNode = YAML::Load(R"(---
- type: i2c
  bus: /dev/i2c-1
  components:
    3b:  # i2c address
      type: pcf8574
      direction: input
      inputs:
        0: io_that_does_not_exist
)");

  REQUIRE_THROWS_AS(HomeAutomation::Runtime::IOFactory::createIOs(
                        ioNode, 0 /* must not be used */, gv),
                    std::invalid_argument);
}
