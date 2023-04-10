#include <mqtt.hpp>

#include <subprocess.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;
using namespace HomeAutomation::IO::MQTT;

static constexpr char const *TOPIC = "/sample/topic";

static int compose_up_mosquitto() {
  return TestUtil::exec("cd ../../../docker && docker compose up -d mosquitto");
}

static int compose_rm_mosquitto() {
  return TestUtil::exec(
      "cd ../../../docker && docker compose rm -sf mosquitto");
}

static bool is_mosquitto_running() {
  return TestUtil::exec(
             "cd ../../../docker && docker compose ps | grep -q mosquitto") ==
         EXIT_SUCCESS;
}

static bool is_mosquitto_not_running_anymore() {
  return !is_mosquitto_running();
}

TEST_CASE("MQTT client") {
  spdlog::cfg::load_env_levels();

  REQUIRE(compose_rm_mosquitto() == EXIT_SUCCESS);
  REQUIRE(TestUtil::poll_for_cond(is_mosquitto_not_running_anymore, 15s,
                                  100ms) == true);
  REQUIRE(compose_up_mosquitto() == EXIT_SUCCESS);
  std::this_thread::sleep_for(200ms);

  SUBCASE("mqtt: instantiate/destruct mqtt client") {
    ClientPaho client{"tcp://mosquitto:1883"};
  }

  SUBCASE("mqtt: connect/disconnect mqtt client") {
    ClientPaho client{"tcp://mosquitto:1883"};
    client.connect();
    client.disconnect();
  }

  SUBCASE("mqtt: receive no mqtt messages") {
    ClientPaho client{"tcp://mosquitto:1883"};
    client.connect();

    auto message = client.receive();
    REQUIRE(!message);

    client.disconnect();
  }

  SUBCASE("mqtt: connect/disconnect mqtt client, no broker listening") {
    // this test shows that we are currently depending on a working/reachable
    // broker
    ClientPaho client{"tcp://mosquitto:1884"};
    REQUIRE_NOTHROW(client.connect());
    REQUIRE_NOTHROW(client.disconnect());
  }

  SUBCASE("mqtt: publish/receive one mqtt message") {
    using namespace std::chrono_literals;

    ClientPaho client_first{"tcp://mosquitto:1883", "client-first"};
    ClientPaho client_second{"tcp://mosquitto:1883", "client-second"};

    client_second.connect();
    client_second.subscribe(TOPIC);

    client_first.connect();
    client_first.send(TOPIC, "sample payload");

    // let the MQTT stack do its things
    std::this_thread::sleep_for(100ms);

    auto message = client_second.receive();
    REQUIRE(message);
    REQUIRE(message->get_topic().c_str() == std::string(TOPIC));
    REQUIRE(message->get_payload().c_str() == std::string("sample payload"));

    client_first.disconnect();
    client_second.disconnect();
  }

  SUBCASE("mqtt: publish/receive mqtt message with flaky broker") {
    using namespace std::chrono_literals;

    ClientPaho client_first{"tcp://mosquitto:1883", "client-first"};
    ClientPaho client_second{"tcp://mosquitto:1883", "client-second"};

    client_second.connect();
    client_second.subscribe(TOPIC);

    client_first.connect();
    client_first.send(TOPIC, "sample payload");

    // let the MQTT stack do its things
    std::this_thread::sleep_for(200ms);

    auto message = client_second.receive();
    REQUIRE(message);
    REQUIRE(message->get_topic() == std::string(TOPIC));
    REQUIRE(message->get_payload() == std::string("sample payload"));

    for (std::size_t cnt = 5; cnt > 0; --cnt) {
      REQUIRE(compose_rm_mosquitto() == EXIT_SUCCESS);
      std::this_thread::sleep_for(1s);
      auto disconnected_payload = fmt::format("disconnected payload {}", cnt);
      spdlog::info("Publishing message with payload \"{}\" to topic {}",
                   disconnected_payload, TOPIC);
      client_first.send(TOPIC, disconnected_payload);
      std::this_thread::sleep_for(1s);
      REQUIRE(compose_up_mosquitto() == EXIT_SUCCESS);
      REQUIRE(TestUtil::poll_for_cond(
          [&client_first, &client_second]() {
            return client_first.is_connected() && client_second.is_connected();
          },
          10s, 100ms));

      auto payload = fmt::format("message payload {}", cnt);
      spdlog::info("Publishing message with payload \"{}\" to topic {}",
                   payload, TOPIC);
      client_first.send(TOPIC, payload);
      // let the MQTT stack do its things
      std::this_thread::sleep_for(300ms);
      message = client_second.receive();
      REQUIRE(message);
      spdlog::info("Received message with payload \"{}\" to topic {}",
                   message->get_payload(), message->get_topic());
      if (message->get_payload() == std::string(disconnected_payload)) {
        message = client_second.receive();
        REQUIRE(message);
        spdlog::info("Received message with payload \"{}\" to topic {}",
                     message->get_payload(), message->get_topic());
      }
      REQUIRE(message->get_topic() == std::string(TOPIC));
      REQUIRE(message->get_payload() == std::string(payload));
    }

    client_first.disconnect();
    client_second.disconnect();
  }
}
