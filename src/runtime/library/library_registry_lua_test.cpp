#include <library_registry_lua.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("library registry lua: create and use triggers", "[single-file]") {
  sol::state lua;
  lua.open_libraries();
  HomeAutomation::Library::LuaLibraryRegistry::RegisterComponents(lua);
  REQUIRE_NOTHROW(lua.script_file("test/test_library.lua"));
  REQUIRE(lua.get<int>("TEST_RESULT") == 0);
}
