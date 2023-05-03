#include <blind.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

using namespace std::literals;

static constexpr auto const cfg = HomeAutomation::Library::BlindConfig{
    .periodIdle = 500ms, .periodUp = 30s, .periodDown = 30s};

TEST_CASE("blind: states of inputs unchanged") {
  spdlog::set_level(spdlog::level::debug);

  auto start = std::chrono::steady_clock::now();
  HomeAutomation::Library::Blind blind(cfg, start);

  auto relay_states = blind.execute(start + 100ms, false, false);
  REQUIRE(relay_states == std::make_tuple(false, false));

  relay_states = blind.execute(start + 200ms, false, false);
  REQUIRE(relay_states == std::make_tuple(false, false));
}

TEST_CASE("blind: move up") {
  spdlog::set_level(spdlog::level::debug);

  auto start = std::chrono::steady_clock::now();
  HomeAutomation::Library::Blind blind_test(cfg, start);

  auto relay_states =
      blind_test.execute(start + cfg.periodIdle + 100ms, false, false);
  REQUIRE(relay_states == std::make_tuple(false, false));

  relay_states =
      blind_test.execute(start + cfg.periodIdle + 200ms, true, false);
  REQUIRE(relay_states == std::make_tuple(true, false));

  relay_states =
      blind_test.execute(start + cfg.periodIdle + 300ms, true, false);
  REQUIRE(relay_states == std::make_tuple(true, false));
}

TEST_CASE("blind: both inputs true") {
  spdlog::set_level(spdlog::level::debug);

  auto start = std::chrono::steady_clock::now();
  HomeAutomation::Library::Blind blind(cfg, start);

  auto relay_states =
      blind.execute(start + cfg.periodIdle + 100ms, false, false);
  REQUIRE(relay_states == std::make_tuple(false, false));

  relay_states = blind.execute(start + cfg.periodIdle + 200ms, true, true);
  REQUIRE(relay_states == std::make_tuple(true, false));

  relay_states = blind.execute(start + cfg.periodIdle + 300ms, true, true);
  REQUIRE(relay_states == std::make_tuple(true, false));
}

TEST_CASE("blind: complete run") {
  spdlog::set_level(spdlog::level::debug);

  auto const start = std::chrono::steady_clock::now();
  HomeAutomation::Library::Blind blind(cfg, start);

  /* idle */
  auto const timestamp_start_idle = start + cfg.periodIdle + 100ms;
  auto relay_states = blind.execute(timestamp_start_idle, false, false);
  REQUIRE(relay_states == std::make_tuple(false, false));

  /* up */
  relay_states = blind.execute(timestamp_start_idle + 100ms, true, false);
  REQUIRE(relay_states == std::make_tuple(true, false));

  relay_states = blind.execute(timestamp_start_idle + 200ms, true, false);
  REQUIRE(relay_states == std::make_tuple(true, false));

  relay_states = blind.execute(timestamp_start_idle + 300ms, false, false);
  REQUIRE(relay_states == std::make_tuple(true, false));

  /* idle */
  auto const timestamp_start_idle_2 = start + cfg.periodIdle + 500ms;
  relay_states = blind.execute(timestamp_start_idle_2, true, false);
  REQUIRE(relay_states == std::make_tuple(false, false));

  relay_states = blind.execute(timestamp_start_idle_2 + 100ms, true, false);
  REQUIRE(relay_states == std::make_tuple(false, false));

  relay_states = blind.execute(timestamp_start_idle_2 + 200ms, false, false);
  REQUIRE(relay_states == std::make_tuple(false, false));

  relay_states = blind.execute(timestamp_start_idle_2 + 300ms, true, false);
  REQUIRE(relay_states == std::make_tuple(false, false));

  /* down */
  auto const timestamp_start_down =
      start + cfg.periodIdle + 500ms + cfg.periodIdle + 100ms;
  relay_states = blind.execute(timestamp_start_down, false, true);
  REQUIRE(relay_states == std::make_tuple(false, true));

  relay_states =
      blind.execute(timestamp_start_down + cfg.periodDown + 100ms, true, false);
  REQUIRE(relay_states == std::make_tuple(false, false));
}

TEST_CASE("blind: two instances") {
  spdlog::set_level(spdlog::level::debug);

  auto const start = std::chrono::steady_clock::now();
  HomeAutomation::Library::Blind blind_1(cfg, start);
  HomeAutomation::Library::Blind blind_2(cfg, start);

  /* idle */
  auto const timestamp_start_idle = start + cfg.periodIdle + 100ms;
  auto relay_states = blind_1.execute(timestamp_start_idle, false, false);
  REQUIRE(relay_states == std::make_tuple(false, false));
  relay_states = blind_2.execute(timestamp_start_idle, false, false);
  REQUIRE(relay_states == std::make_tuple(false, false));

  /* up */
  relay_states = blind_1.execute(timestamp_start_idle + 100ms, true, false);
  REQUIRE(relay_states == std::make_tuple(true, false));
  relay_states = blind_2.execute(timestamp_start_idle + 100ms, true, false);
  REQUIRE(relay_states == std::make_tuple(true, false));

  relay_states = blind_1.execute(timestamp_start_idle + 200ms, true, false);
  REQUIRE(relay_states == std::make_tuple(true, false));
  relay_states = blind_2.execute(timestamp_start_idle + 200ms, true, false);
  REQUIRE(relay_states == std::make_tuple(true, false));

  relay_states = blind_1.execute(timestamp_start_idle + 300ms, false, false);
  REQUIRE(relay_states == std::make_tuple(true, false));
  relay_states = blind_2.execute(timestamp_start_idle + 300ms, false, false);
  REQUIRE(relay_states == std::make_tuple(true, false));

  /* idle */
  auto const timestamp_start_idle_2 = start + cfg.periodIdle + 500ms;
  relay_states = blind_1.execute(timestamp_start_idle_2, true, false);
  REQUIRE(relay_states == std::make_tuple(false, false));
  relay_states = blind_2.execute(timestamp_start_idle_2, true, false);
  REQUIRE(relay_states == std::make_tuple(false, false));

  relay_states = blind_1.execute(timestamp_start_idle_2 + 100ms, true, false);
  REQUIRE(relay_states == std::make_tuple(false, false));
  relay_states = blind_2.execute(timestamp_start_idle_2 + 100ms, true, false);
  REQUIRE(relay_states == std::make_tuple(false, false));

  relay_states = blind_1.execute(timestamp_start_idle_2 + 200ms, false, false);
  REQUIRE(relay_states == std::make_tuple(false, false));
  relay_states = blind_2.execute(timestamp_start_idle_2 + 200ms, false, false);
  REQUIRE(relay_states == std::make_tuple(false, false));

  relay_states = blind_1.execute(timestamp_start_idle_2 + 300ms, true, false);
  REQUIRE(relay_states == std::make_tuple(false, false));
  relay_states = blind_2.execute(timestamp_start_idle_2 + 300ms, true, false);
  REQUIRE(relay_states == std::make_tuple(false, false));

  /* down */
  auto const timestamp_start_down =
      start + cfg.periodIdle + 500ms + cfg.periodIdle + 100ms;
  relay_states = blind_1.execute(timestamp_start_down, false, true);
  REQUIRE(relay_states == std::make_tuple(false, true));
  relay_states = blind_2.execute(timestamp_start_down, false, true);
  REQUIRE(relay_states == std::make_tuple(false, true));

  relay_states = blind_1.execute(timestamp_start_down + cfg.periodDown + 100ms,
                                 true, false);
  REQUIRE(relay_states == std::make_tuple(false, false));
  relay_states = blind_2.execute(timestamp_start_down + cfg.periodDown + 100ms,
                                 true, false);
  REQUIRE(relay_states == std::make_tuple(false, false));
}
