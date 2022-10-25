#include <blind.hpp>

#include <catch2/catch_all.hpp>

using namespace std::literals;

static constexpr auto const cfg = HomeAutomation::Components::BlindConfig{
    .periodIdle = 500ms, .periodUp = 1s, .periodDown = 1s};

TEST_CASE("state outputs", "[single-file]") {
  auto start = std::chrono::high_resolution_clock::now();
  REQUIRE(
      HomeAutomation::Components::BlindStateIdle(cfg, start).getStateData() ==
      HomeAutomation::Components::BlindOutputs{false, false});
  REQUIRE(HomeAutomation::Components::BlindStateUp(
              cfg, std::chrono::high_resolution_clock::now(), true, true)
              .getStateData() ==
          HomeAutomation::Components::BlindOutputs{true, false});
  REQUIRE(HomeAutomation::Components::BlindStateDown(
              cfg, std::chrono::high_resolution_clock::now(), true, true)
              .getStateData() ==
          HomeAutomation::Components::BlindOutputs{false, true});
}

TEST_CASE("stay in idle state", "[single-file]") {
  auto start = std::chrono::high_resolution_clock::now();
  auto idle =
      std::make_unique<HomeAutomation::Components::BlindStateIdle>(cfg, start);

  auto next = idle->execute(start + 100ms, false, false);

  REQUIRE(next.has_value() == false);
}

TEST_CASE("transition into up state", "[single-file]") {
  auto start = std::chrono::high_resolution_clock::now();
  auto idle =
      std::make_unique<HomeAutomation::Components::BlindStateIdle>(cfg, start);

  auto next = idle->execute(start + cfg.periodIdle + 100ms, true, false);

  REQUIRE(next.has_value() == true);
  REQUIRE(next.value()->id() == "up");
}

TEST_CASE("transition into up state with both inputs true", "[single-file]") {
  auto start = std::chrono::high_resolution_clock::now();
  auto idle =
      std::make_unique<HomeAutomation::Components::BlindStateIdle>(cfg, start);

  auto next = idle->execute(start + cfg.periodIdle + 100ms, true, true);

  REQUIRE(next.has_value() == true);
  REQUIRE(next.value()->id() == "up");
}

TEST_CASE("transition into down state", "[single-file]") {
  auto start = std::chrono::high_resolution_clock::now();
  auto idle =
      std::make_unique<HomeAutomation::Components::BlindStateIdle>(cfg, start);

  auto next = idle->execute(start + cfg.periodIdle + 100ms, false, true);

  REQUIRE(next.has_value() == true);
  REQUIRE(next.value()->id() == "down");
}

TEST_CASE("transition from up state to idle as time is over", "[single-file]") {
  auto start = std::chrono::high_resolution_clock::now();
  auto up = std::make_unique<HomeAutomation::Components::BlindStateUp>(
      cfg, std::chrono::high_resolution_clock::now(), true, false);

  auto next = up->execute(start, false, false);

  REQUIRE(next.has_value() == false);

  next = up->execute(start + 100ms, false, false);

  REQUIRE(next.has_value() == false);

  next = up->execute(start + 1100ms, false, false);

  REQUIRE(next.has_value() == true);
  REQUIRE(next.value()->id() == "idle");
}
