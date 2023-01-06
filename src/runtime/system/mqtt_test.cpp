#include <mqtt.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

using namespace HomeAutomation::IO::MQTT;

TEST_CASE("mqtt: instantiate/destruct mqtt client") {
  ClientPaho client{"tcp://localhost:1883"};
}

TEST_CASE("mqtt: connect/disconnect mqtt client") {
  ClientPaho client{"tcp://localhost:1883"};
  client.connect();
  client.disconnect();
}

TEST_CASE("mqtt: receive no mqtt messages") {
  ClientPaho client{"tcp://localhost:1883"};
  client.connect();

  auto message = client.receive();
  REQUIRE(!message);

  client.disconnect();
}

TEST_CASE("mqtt: connect/disconnect mqtt client, no broker listening") {
  // this test shows that we are currently depending on a working/reachable
  // broker
  ClientPaho client{"tcp://localhost:1884"};
  REQUIRE_NOTHROW(client.connect());
  REQUIRE_NOTHROW(client.disconnect());
}

TEST_CASE("mqtt: publish/receive one mqtt message") {
  using namespace std::chrono_literals;

  ClientPaho client_first{"tcp://localhost:1883", "client-first"};
  ClientPaho client_second{"tcp://localhost:1883", "client-second"};

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
