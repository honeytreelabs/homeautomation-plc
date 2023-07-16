#include "mqtt.hpp"
#include <mqtt_c.hpp>

#include <subprocess.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <condition_variable>
#include <mutex>

using namespace std::chrono_literals;
using namespace HomeAutomation::IO::MQTT;

static constexpr char const *TOPIC = "MQTT Examples";

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
    Client client{
        std::make_unique<ClientPahoC>("tcp://localhost:1883", "testclient")};
  }

  SUBCASE("mqtt: connect/disconnect mqtt client") {
    Client client{
        std::make_unique<ClientPahoC>("tcp://localhost:1883", "testclient")};
    client.connect();
    client.disconnect();
  }

  SUBCASE("mqtt: receive no mqtt messages") {
    Client client{
        std::make_unique<ClientPahoC>("tcp://localhost:1883", "testclient")};
    client.connect();

    auto message = client.receive();
    REQUIRE(!message);

    client.disconnect();
  }

  SUBCASE("mqtt: connect/disconnect mqtt client, no broker listening") {
    // this test shows that we are currently depending on a working/reachable
    // broker
    Client client{
        std::make_unique<ClientPahoC>("tcp://localhost:1883", "testclient")};
    REQUIRE_NOTHROW(client.connect());
    REQUIRE_NOTHROW(client.disconnect());
  }

  SUBCASE("mqtt: publish/receive one mqtt message") {
    using namespace std::chrono_literals;

    Client client_first{
        std::make_unique<ClientPahoC>("tcp://localhost:1883", "client-first")};
    Client client_second{
        std::make_unique<ClientPahoC>("tcp://localhost:1883", "client-second")};

    client_second.connect();
    client_second.subscribe(TOPIC);

    client_first.connect();
    std::this_thread::sleep_for(1s);
    client_first.publish(TOPIC, "sample payload");

    // let the MQTT stack do its things
    std::this_thread::sleep_for(1s);

    auto message = client_second.receive();
    REQUIRE(message);
    REQUIRE(message.value().topic() == std::string(TOPIC));
    REQUIRE(message.value().payload_str() == "sample payload");

    client_first.disconnect();
    client_second.disconnect();
  }

  SUBCASE(
      "mqtt: connect to stopped broker then publish/receive one mqtt message") {
    using namespace std::chrono_literals;

    REQUIRE(compose_rm_mosquitto() == EXIT_SUCCESS);
    REQUIRE(TestUtil::poll_for_cond(is_mosquitto_not_running_anymore, 15s,
                                    100ms) == true);

    Client client_first{
        std::make_unique<ClientPahoC>("tcp://localhost:1883", "client-first")};
    Client client_second{
        std::make_unique<ClientPahoC>("tcp://localhost:1883", "client-second")};

    client_second.connect();
    client_second.subscribe(TOPIC);

    client_first.connect();
    std::this_thread::sleep_for(1s);

    REQUIRE(compose_up_mosquitto() == EXIT_SUCCESS);
    std::this_thread::sleep_for(1s);

    client_first.publish(TOPIC, "sample payload");

    // let the MQTT stack do its things
    std::this_thread::sleep_for(1s);

    auto message = client_second.receive();
    REQUIRE(message);
    REQUIRE(message.value().topic() == std::string(TOPIC));
    REQUIRE(message.value().payload_str() == "sample payload");

    client_first.disconnect();
    client_second.disconnect();
  }

  SUBCASE("mqtt: publish/receive mqtt message with flaky broker") {
    using namespace std::chrono_literals;

    spdlog::info("Start flaky broker subcase");

    Client client_first{
        std::make_unique<ClientPahoC>("tcp://localhost:1883", "client-first")};
    Client client_second{
        std::make_unique<ClientPahoC>("tcp://localhost:1883", "client-second")};

    client_second.connect();
    client_second.subscribe(TOPIC);

    client_first.connect();
    std::this_thread::sleep_for(1s);
    client_first.publish(TOPIC, "sample payload");

    // let the MQTT stack do its things
    std::this_thread::sleep_for(1s);

    auto message = client_second.receive();
    REQUIRE(message);
    REQUIRE(message.value().topic() == std::string(TOPIC));
    REQUIRE(message.value().payload_str() == "sample payload");

    spdlog::info("Starting flaky-loop.");
    for (std::size_t cnt = 5; cnt > 0; --cnt) {
      REQUIRE(compose_rm_mosquitto() == EXIT_SUCCESS);
      TestUtil::poll_for_cond(is_mosquitto_not_running_anymore, 15s, 100ms);
      auto disconnected_payload = fmt::format("disconnected payload {}", cnt);
      client_first.publish(TOPIC, disconnected_payload);
      std::this_thread::sleep_for(1s);
      std::mutex m;
      std::unique_lock lk{m};
      std::condition_variable cv;
      std::atomic_bool has_subscribed{false};
      client_second.set_on_resubscribe(
          [&cv, &has_subscribed](SubscribedTopic const &topic) {
            spdlog::info("Resubscription on topic {}", topic.topic);
            if (topic.topic == TOPIC) {
              has_subscribed = true;
              cv.notify_all();
            }
          });
      REQUIRE(compose_up_mosquitto() == EXIT_SUCCESS);

      auto wait_result = cv.wait_for(
          lk, 10s, [&has_subscribed]() -> bool { return has_subscribed; });
      if (!wait_result) {
        spdlog::info("MQTT client has not resubscribed.");
      }
      lk.unlock(); // lock is not needed for the following assertions

      std::this_thread::sleep_for(2s);
      auto payload = fmt::format("message payload {}", cnt);
      client_first.publish(TOPIC, payload);
      // let the MQTT stack do its things
      std::this_thread::sleep_for(300ms);
      message = client_second.receive();
      REQUIRE(message);
      if (message.value().payload_str() == disconnected_payload) {
        message = client_second.receive();
        REQUIRE(message);
      }
      REQUIRE(message.value().topic() == std::string(TOPIC));
      REQUIRE(message.value().payload_str() == payload);
    }

    client_first.disconnect();
    client_second.disconnect();
  }
}
