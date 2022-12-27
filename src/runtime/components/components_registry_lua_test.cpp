#include <components_registry_lua.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("components registry lua: create and use triggers", "[single-file]") {
  sol::state lua;
  lua.open_libraries();
  HomeAutomation::Components::LuaComponentsRegistry::RegisterComponents(lua);
  REQUIRE_NOTHROW(lua.script_file("test/test_library.lua"));
  REQUIRE(lua.get<int>("TEST_RESULT") == 0);
}
