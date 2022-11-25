#include <lua.hpp>

#include <catch2/catch_all.hpp>

TEST_CASE("execute simple lua script", "[single-file]") {
  REQUIRE(true == true);
}
