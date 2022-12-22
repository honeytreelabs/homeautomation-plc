#include <blind.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace std::literals;

static constexpr auto const cfg = HomeAutomation::Components::BlindConfig{
    .periodIdle = 500ms, .periodUp = 30s, .periodDown = 30s};

TEST_CASE("blind: states of inputs unchanged", "[single-file]") {
  spdlog::set_level(spdlog::level::debug);

  auto start = std::chrono::high_resolution_clock::now();
  HomeAutomation::Components::Blind blind(cfg, start);

  auto relay_states = blind.execute(start + 100ms, false, false);
  REQUIRE(relay_states.up == false);
  REQUIRE(relay_states.down == false);

  relay_states = blind.execute(start + 200ms, false, false);
  REQUIRE(relay_states.up == false);
  REQUIRE(relay_states.down == false);
}

TEST_CASE("blind: move up", "[single-file]") {
  spdlog::set_level(spdlog::level::debug);

  auto start = std::chrono::high_resolution_clock::now();
  HomeAutomation::Components::Blind blind_test(cfg, start);

  auto relay_states =
      blind_test.execute(start + cfg.periodIdle + 100ms, false, false);
  REQUIRE(relay_states.up == false);
  REQUIRE(relay_states.down == false);

  relay_states =
      blind_test.execute(start + cfg.periodIdle + 200ms, true, false);
  REQUIRE(relay_states.up == true);
  REQUIRE(relay_states.down == false);

  relay_states =
      blind_test.execute(start + cfg.periodIdle + 300ms, true, false);
  REQUIRE(relay_states.up == true);
  REQUIRE(relay_states.down == false);
}

TEST_CASE("blind: both inputs true", "[single-file]") {
  spdlog::set_level(spdlog::level::debug);

  auto start = std::chrono::high_resolution_clock::now();
  HomeAutomation::Components::Blind blind(cfg, start);

  auto relay_states =
      blind.execute(start + cfg.periodIdle + 100ms, false, false);
  REQUIRE(relay_states.up == false);
  REQUIRE(relay_states.down == false);

  relay_states = blind.execute(start + cfg.periodIdle + 200ms, true, true);
  REQUIRE(relay_states.up == true);
  REQUIRE(relay_states.down == false);

  relay_states = blind.execute(start + cfg.periodIdle + 300ms, true, true);
  REQUIRE(relay_states.up == true);
  REQUIRE(relay_states.down == false);
}

TEST_CASE("blind: complete run", "[single-file]") {
  spdlog::set_level(spdlog::level::debug);

  auto start = std::chrono::high_resolution_clock::now();
  HomeAutomation::Components::Blind blind(cfg, start);

  /* idle */
  auto const timestamp_start_idle = start + cfg.periodIdle + 100ms;
  auto relay_states = blind.execute(timestamp_start_idle, false, false);
  REQUIRE(relay_states.up == false);
  REQUIRE(relay_states.down == false);

  /* up */
  relay_states = blind.execute(timestamp_start_idle + 100ms, true, false);
  REQUIRE(relay_states.up == true);
  REQUIRE(relay_states.down == false);

  relay_states = blind.execute(timestamp_start_idle + 200ms, true, false);
  REQUIRE(relay_states.up == true);
  REQUIRE(relay_states.down == false);

  relay_states = blind.execute(timestamp_start_idle + 300ms, false, false);
  REQUIRE(relay_states.up == true);
  REQUIRE(relay_states.down == false);

  /* idle */
  auto const timestamp_start_idle_2 = start + cfg.periodIdle + 500ms;
  relay_states = blind.execute(timestamp_start_idle_2, true, false);
  REQUIRE(relay_states.up == false);
  REQUIRE(relay_states.down == false);

  relay_states = blind.execute(timestamp_start_idle_2 + 100ms, true, false);
  REQUIRE(relay_states.up == false);
  REQUIRE(relay_states.down == false);

  relay_states = blind.execute(timestamp_start_idle_2 + 200ms, false, false);
  REQUIRE(relay_states.up == false);
  REQUIRE(relay_states.down == false);

  relay_states = blind.execute(timestamp_start_idle_2 + 300ms, true, false);
  REQUIRE(relay_states.up == false);
  REQUIRE(relay_states.down == false);

  /* down */
  auto const timestamp_start_down =
      start + cfg.periodIdle + 500ms + cfg.periodIdle + 100ms;
  relay_states = blind.execute(timestamp_start_down, false, true);
  REQUIRE(relay_states.up == false);
  REQUIRE(relay_states.down == true);

  relay_states =
      blind.execute(timestamp_start_down + cfg.periodDown + 100ms, true, false);
  REQUIRE(relay_states.up == false);
  REQUIRE(relay_states.down == false);
}
