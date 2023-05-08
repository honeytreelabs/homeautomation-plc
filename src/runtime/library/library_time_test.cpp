#include <time.hpp>

#include <sol/sol.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <spdlog/spdlog.h>

using namespace std::literals;

TEST_CASE("lua program: window blind complete run") {
  sol::state lua;
  lua.open_libraries(sol::lib::base);
  HomeAutomation::Library::Util::RegisterComponent(lua);

  HomeAutomation::TimeStamp ts_1 = std::chrono::steady_clock::now();
  auto ts_2 = ts_1 + 1s + 100ms;
  lua["ts_1"] = ts_1;
  lua["ts_2"] = ts_2;

  lua.script(
      "diff = to_millis_since_start(ts_2) - to_millis_since_start(ts_1)");
  REQUIRE(lua.get<std::int64_t>("diff") == 1100);

  lua.script(
      "diff = to_millis_since_start(ts_1) - to_millis_since_start(ts_2)");
  REQUIRE(lua.get<std::int64_t>("diff") == -1100);
}
