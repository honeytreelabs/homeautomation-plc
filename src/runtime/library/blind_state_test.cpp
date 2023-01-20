#include <blind_state.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

using namespace std::literals;

static constexpr auto const cfg = HomeAutomation::Library::BlindConfig{
    .periodIdle = 500ms, .periodUp = 1s, .periodDown = 1s};

TEST_CASE("blinds state: state outputs") {
  auto start = std::chrono::steady_clock::now();
  REQUIRE(HomeAutomation::Library::BlindStateIdle(cfg, start).getStateData() ==
          HomeAutomation::Library::BlindOutputs{false, false});
  REQUIRE(HomeAutomation::Library::BlindStateUp(
              cfg, std::chrono::steady_clock::now(), true, true)
              .getStateData() ==
          HomeAutomation::Library::BlindOutputs{true, false});
  REQUIRE(HomeAutomation::Library::BlindStateDown(
              cfg, std::chrono::steady_clock::now(), true, true)
              .getStateData() ==
          HomeAutomation::Library::BlindOutputs{false, true});
}

TEST_CASE("blinds state: stay in idle state") {
  auto start = std::chrono::steady_clock::now();
  auto idle =
      std::make_unique<HomeAutomation::Library::BlindStateIdle>(cfg, start);

  auto next = idle->execute(start + 100ms, false, false);

  REQUIRE(next.has_value() == false);
}

TEST_CASE("blinds state: transition into up state") {
  auto start = std::chrono::steady_clock::now();
  auto idle =
      std::make_unique<HomeAutomation::Library::BlindStateIdle>(cfg, start);

  auto next = idle->execute(start + cfg.periodIdle + 100ms, true, false);

  REQUIRE(next.has_value() == true);
  REQUIRE(next.value()->id() == "up");
}

TEST_CASE("blind state: transition into up state with both inputs true") {
  auto start = std::chrono::steady_clock::now();
  auto idle =
      std::make_unique<HomeAutomation::Library::BlindStateIdle>(cfg, start);

  auto next = idle->execute(start + cfg.periodIdle + 100ms, true, true);

  REQUIRE(next.has_value() == true);
  REQUIRE(next.value()->id() == "up");
}

TEST_CASE("blind state: transition into down state") {
  auto start = std::chrono::steady_clock::now();
  auto idle =
      std::make_unique<HomeAutomation::Library::BlindStateIdle>(cfg, start);

  auto next = idle->execute(start + cfg.periodIdle + 100ms, false, true);

  REQUIRE(next.has_value() == true);
  REQUIRE(next.value()->id() == "down");
}

TEST_CASE("blind stae: transition from up state to idle as time is over") {
  auto start = std::chrono::steady_clock::now();
  auto up = std::make_unique<HomeAutomation::Library::BlindStateUp>(
      cfg, std::chrono::steady_clock::now(), true, false);

  auto next = up->execute(start, false, false);

  REQUIRE(next.has_value() == false);

  next = up->execute(start + 100ms, false, false);

  REQUIRE(next.has_value() == false);

  next = up->execute(start + 1100ms, false, false);

  REQUIRE(next.has_value() == true);
  REQUIRE(next.value()->id() == "idle");
}
