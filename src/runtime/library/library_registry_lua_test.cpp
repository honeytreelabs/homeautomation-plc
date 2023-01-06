#include <library_registry_lua.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

TEST_CASE("library registry lua: create and use triggers") {
  sol::state lua;
  lua.open_libraries();
  HomeAutomation::Library::LuaLibraryRegistry::RegisterComponents(lua);
  REQUIRE_NOTHROW(lua.script_file("test/test_library.lua"));
  REQUIRE(lua.get<int>("TEST_RESULT") == 0);
}
