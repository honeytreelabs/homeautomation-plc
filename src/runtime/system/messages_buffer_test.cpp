#include <mqtt.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <sstream>

using namespace HomeAutomation::IO::MQTT;

TEST_CASE("messages buffer: empty") {
  Messages buf;
  REQUIRE(buf.empty() == true);
}

TEST_CASE("messages buffer: get") {
  Messages buf;
  auto elem = buf.get();
  REQUIRE(elem == std::nullopt);
}

TEST_CASE("messages buffer: get_for empty") {
  using namespace std::chrono_literals;

  Messages buf;
  auto elem = buf.get_for(100ms);
  REQUIRE(elem == std::nullopt);
}

TEST_CASE("messages buffer: get_for existing") {
  using namespace std::chrono_literals;

  Messages buf;
  std::thread t1{[&buf]() {
    auto elem = buf.get_for(1s);
    REQUIRE(elem.has_value());
    REQUIRE(elem.value()->get_topic() == std::string("/sample/topic"));
    REQUIRE(elem.value()->get_payload().c_str() ==
            std::string("sample payload"));
  }};
  buf.put(mqtt::make_message("/sample/topic", "sample payload"));
  t1.join();
}

TEST_CASE("messages buffer: get_for existing multi") {
  using namespace std::chrono_literals;

  constexpr int ROUNDS = 100;

  Messages buf;
  std::thread t1{[&buf]() {
    for (int i = 0; i < ROUNDS; i++) {
      auto elem = buf.get_for(1s);
      REQUIRE(elem.has_value());

      REQUIRE(elem.value()->get_topic() == std::string("/sample/topic"));

      std::stringstream payload;
      payload << "round ";
      payload << i;
      REQUIRE(elem.value()->get_payload().c_str() ==
              std::string(payload.str().c_str()));
    }
  }};
  for (int i = 0; i < ROUNDS; i++) {
    std::stringstream payload;
    payload << "round ";
    payload << i;
    buf.put(mqtt::make_message("/sample/topic", payload.str().c_str()));
  }
  t1.join();
}
