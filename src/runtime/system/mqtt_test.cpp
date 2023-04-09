#include <mqtt.hpp>

#include <subprocess.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;
using namespace HomeAutomation::IO::MQTT;

static int compose_up_mosquitto() {
  return TestUtil::exec("cd ../../../docker && docker compose up -d mosquitto");
}

static int compose_rm_mosquitto() {
  return TestUtil::exec(
      "cd ../../../docker && docker compose rm -sf mosquitto");
}

static bool is_mosquitto_not_running_anymore() {
  return TestUtil::exec("docker ps -a | grep -vq mosquitto") == EXIT_SUCCESS;
}

TEST_CASE("MQTT client") {
  auto logger = spdlog::stdout_color_mt("console");
  spdlog::cfg::load_env_levels();

  REQUIRE(compose_rm_mosquitto() == EXIT_SUCCESS);
  REQUIRE(
      TestUtil::poll_for_cond(is_mosquitto_not_running_anymore, 150, 100ms));
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
    client_second.subscribe("/sample/topic");

    client_first.connect();
    client_first.send("/sample/topic", "sample payload");

    // let the MQTT stack do its things
    std::this_thread::sleep_for(100ms);

    auto message = client_second.receive();
    REQUIRE(message);
    REQUIRE(message->get_topic().c_str() == std::string("/sample/topic"));
    REQUIRE(message->get_payload().c_str() == std::string("sample payload"));

    client_first.disconnect();
    client_second.disconnect();
  }

  SUBCASE("mqtt: publish/receive mqtt message with flaky broker") {
    using namespace std::chrono_literals;

    ClientPaho client_first{"tcp://mosquitto:1883", "client-first"};
    ClientPaho client_second{"tcp://mosquitto:1883", "client-second"};

    client_second.connect();
    client_second.subscribe("/sample/topic");

    client_first.connect();
    client_first.send("/sample/topic", "sample payload");

    // let the MQTT stack do its things
    std::this_thread::sleep_for(200ms);

    auto message = client_second.receive();
    REQUIRE(message);
    REQUIRE(message->get_topic() == std::string("/sample/topic"));
    REQUIRE(message->get_payload() == std::string("sample payload"));

    for (std::size_t cnt = 5; cnt > 0; --cnt) {
      REQUIRE(compose_rm_mosquitto() == EXIT_SUCCESS);
      std::this_thread::sleep_for(1s);
      constexpr char const *PAYLOAD = "sample payload";
      constexpr char const *TOPIC = "/sample/topic";
      logger->info("Publishing message with payload \"{}\" to topic {}",
                   PAYLOAD, TOPIC);
      client_first.send(TOPIC, PAYLOAD);
      std::this_thread::sleep_for(1s);
      REQUIRE(compose_up_mosquitto() == EXIT_SUCCESS);
      REQUIRE(TestUtil::poll_for_cond(
          [&client_first, &client_second]() {
            return client_first.is_connected() && client_second.is_connected();
          },
          100, 100ms));

      // let the MQTT stack do its things
      std::this_thread::sleep_for(300ms);
      message = client_second.receive();
      REQUIRE(message);
      logger->info("Received message with payload \"{}\" to topic {}",
                   message->get_payload(), message->get_topic());
      REQUIRE(message->get_topic() == std::string(TOPIC));
      REQUIRE(message->get_payload() == std::string(PAYLOAD));
    }

    client_first.disconnect();
    client_second.disconnect();
  }
}
