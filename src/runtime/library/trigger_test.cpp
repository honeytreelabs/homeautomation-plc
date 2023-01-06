#include <trigger.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

TEST_CASE("trigger: rising trigger normal operation") {
  HomeAutomation::Library::R_TRIG trigger;
  auto state = trigger.execute(false);
  REQUIRE(state == false);
  state = trigger.execute(true);
  REQUIRE(state == true);
  state = trigger.execute(true);
  REQUIRE(state == false);
  state = trigger.execute(true);
  REQUIRE(state == false);
  state = trigger.execute(false);
  REQUIRE(state == false);
  state = trigger.execute(true);
  REQUIRE(state == true);
}

TEST_CASE("trigger: rising trigger input already true") {
  HomeAutomation::Library::R_TRIG trigger(true);
  auto state = trigger.execute(true);
  REQUIRE(state == false);
  state = trigger.execute(false);
  REQUIRE(state == false);
  state = trigger.execute(true);
  REQUIRE(state == true);
  state = trigger.execute(true);
  REQUIRE(state == false);
  state = trigger.execute(true);
  REQUIRE(state == false);
  state = trigger.execute(false);
  REQUIRE(state == false);
  state = trigger.execute(true);
  REQUIRE(state == true);
}

TEST_CASE("trigger: falling trigger normal operation") {
  HomeAutomation::Library::F_TRIG trigger;
  auto state = trigger.execute(false);
  REQUIRE(state == false);
  state = trigger.execute(true);
  REQUIRE(state == false);
  state = trigger.execute(true);
  REQUIRE(state == false);
  state = trigger.execute(true);
  REQUIRE(state == false);
  state = trigger.execute(false);
  REQUIRE(state == true);
  state = trigger.execute(true);
  REQUIRE(state == false);
}

TEST_CASE("trigger: falling trigger input already true") {
  HomeAutomation::Library::F_TRIG trigger(true);
  auto state = trigger.execute(true);
  REQUIRE(state == false);
  state = trigger.execute(false);
  REQUIRE(state == true);
  state = trigger.execute(true);
  REQUIRE(state == false);
  state = trigger.execute(true);
  REQUIRE(state == false);
  state = trigger.execute(true);
  REQUIRE(state == false);
  state = trigger.execute(false);
  REQUIRE(state == true);
  state = trigger.execute(true);
  REQUIRE(state == false);
}
