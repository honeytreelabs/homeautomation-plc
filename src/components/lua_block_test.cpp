#include <lua_block.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace HomeAutomation;
using namespace HomeAutomation::Components;

TEST_CASE("execute simple lua script", "[single-file]") {
  VarValue in_i{std::in_place_type<int>, 42};
  VarValue in_b{std::in_place_type<bool>, true};
  VarValue out_i{std::in_place_type<int>, 23};
  VarValue out_b{std::in_place_type<bool>, false};
  GV gv{};
  gv.inputs.emplace("i", std::move(in_i));
  gv.inputs.emplace("b", std::move(in_b));
  gv.outputs.emplace("i", std::move(out_i));
  gv.outputs.emplace("b", std::move(out_b));
  Lua lua_block("test/assignment.lua");

  lua_block.execute(gv);

  REQUIRE(std::get<int>(gv.inputs["i"]) == 43);
  REQUIRE(std::get<bool>(gv.inputs["b"]) == false);
  REQUIRE(std::get<int>(gv.outputs["i"]) == 22);
  REQUIRE(std::get<bool>(gv.outputs["b"]) == true);

  lua_block.execute(gv);

  REQUIRE(std::get<int>(gv.inputs["i"]) == 44);
  REQUIRE(std::get<bool>(gv.inputs["b"]) == true);
  REQUIRE(std::get<int>(gv.outputs["i"]) == 21);
  REQUIRE(std::get<bool>(gv.outputs["b"]) == false);
}
