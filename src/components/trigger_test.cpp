#include <trigger.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("rising trigger normal operation", "[single-file]") {
  HomeAutomation::Components::R_TRIG trigger;
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

TEST_CASE("rising trigger input already true", "[single-file]") {
  HomeAutomation::Components::R_TRIG trigger(true);
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

TEST_CASE("falling trigger normal operation", "[single-file]") {
  HomeAutomation::Components::F_TRIG trigger;
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

TEST_CASE("falling trigger input already true", "[single-file]") {
  HomeAutomation::Components::F_TRIG trigger(true);
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
