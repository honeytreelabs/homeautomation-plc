#include "catch2/catch_test_macros.hpp"
#include <mqtt.hpp>

#include <catch2/catch_all.hpp>

using namespace HomeAutomation::Components::MQTT;

TEST_CASE("instantiate/destruct mqtt client", "[single-file]") {
  Client client{"tcp://localhost:1883"};
}

TEST_CASE("connect/disconnect mqtt client", "[single-file]") {
  Client client{"tcp://localhost:1883"};
  client.connect();
  client.disconnect();
}

TEST_CASE("receive no mqtt messages", "[single-file]") {
  Client client{"tcp://localhost:1883"};
  client.connect();

  auto message = client.receive();
  REQUIRE(!message);

  client.disconnect();
}

TEST_CASE("connect/disconnect mqtt client, no broker listening",
          "[single-file]") {
  // this test shows that we are currently depending on a working/reachable
  // broker
  Client client{"tcp://localhost:1884"};
  REQUIRE_THROWS_AS(client.connect(), mqtt::exception);
  REQUIRE_THROWS_AS(client.disconnect(), mqtt::exception);
}

TEST_CASE("publish/receive one mqtt message", "[single-file]") {
  using namespace std::chrono_literals;

  Client client_first{"tcp://localhost:1883", "client-first"};
  Client client_second{"tcp://localhost:1883", "client-second"};

  client_second.connect();
  client_second.subscribe("/sample/topic");

  client_first.connect();
  client_first.send("/sample/topic", "sample payload");

  // let the MQTT stack do its things
  std::this_thread::sleep_for(100ms);

  auto message = client_second.receive();
  REQUIRE(message);
  REQUIRE_THAT(message->get_topic().c_str(),
               Catch::Matchers::Equals("/sample/topic"));
  REQUIRE_THAT(message->get_payload().c_str(),
               Catch::Matchers::Equals("sample payload"));

  client_first.disconnect();
  client_second.disconnect();
}
