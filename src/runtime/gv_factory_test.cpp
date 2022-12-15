#include <gv_factory.hpp>

#include <yaml-cpp/yaml.h>

#include <catch2/catch_test_macros.hpp>

using namespace HomeAutomation::Runtime;

TEST_CASE("instantiate GVs from YAML", "[single-file]") {
  std::string yaml = R"(---
global_vars:
  inputs:
    one:
      type: bool
      init_val: false
    two:
      type: bool
      init_val: false
    three:
      type: bool
      init_val: true
    four:
      type: bool
      init_val: true
  outputs:
    five:
      type: bool
      init_val: false
    six:
      type: bool
      init_val: false
    seven:
      type: bool
      init_val: true
    eight:
      type: bool
      init_val: true
    nine:
      type: bool
      init_val: true
)";

  YAML::Node root_node = YAML::Load(yaml);

  auto gv = GVFactory::generateGVs(root_node["global_vars"]);

  REQUIRE(std::get<bool>(gv.inputs["one"]) == false);
  REQUIRE(std::get<bool>(gv.inputs["two"]) == false);
  REQUIRE(std::get<bool>(gv.inputs["three"]) == true);
  REQUIRE(std::get<bool>(gv.inputs["four"]) == true);
  REQUIRE(std::get<bool>(gv.outputs["five"]) == false);
  REQUIRE(std::get<bool>(gv.outputs["six"]) == false);
  REQUIRE(std::get<bool>(gv.outputs["seven"]) == true);
  REQUIRE(std::get<bool>(gv.outputs["eight"]) == true);
  REQUIRE(std::get<bool>(gv.outputs["nine"]) == true);
}
