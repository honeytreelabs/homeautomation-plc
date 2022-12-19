#include <gv_factory.hpp>

#include <yaml-cpp/yaml.h>

#include <catch2/catch_test_macros.hpp>

using namespace HomeAutomation::Runtime;

TEST_CASE("initialize GVs from YAML", "[single-file]") {
  HomeAutomation::GV gv;
  gv.inputs["three"] = false;
  gv.inputs["four"] = false;
  gv.outputs["seven"] = false;
  gv.outputs["eight"] = false;
  gv.outputs["nine"] = false;

  std::string yaml = R"(---
global_vars:
  inputs:
    three:
      init_val: true
    four:
      init_val: true
  outputs:
    seven:
      init_val: true
    eight:
      init_val: true
    nine:
      init_val: true
)";

  YAML::Node root_node = YAML::Load(yaml);

  GVFactory::initializeGVs(root_node["global_vars"], &gv);

  REQUIRE(std::get<bool>(gv.inputs["three"]) == true);
  REQUIRE(std::get<bool>(gv.inputs["four"]) == true);
  REQUIRE(std::get<bool>(gv.outputs["seven"]) == true);
  REQUIRE(std::get<bool>(gv.outputs["eight"]) == true);
  REQUIRE(std::get<bool>(gv.outputs["nine"]) == true);
}
