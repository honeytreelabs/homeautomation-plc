#include <mqtt.hpp>

#include <catch2/catch_all.hpp>

#include <sstream>

using namespace Homeautomation::Components::MQTT;

TEST_CASE("messages buffer empty", "[single-file]") {
  Messages buf;
  REQUIRE(buf.empty() == true);
}

TEST_CASE("messages buffer get", "[single-file]") {
  Messages buf;
  auto elem = buf.get();
  REQUIRE(elem == std::nullopt);
}

TEST_CASE("messages buffer get_for empty", "[single-file]") {
  using namespace std::chrono_literals;

  Messages buf;
  auto elem = buf.get_for(100ms);
  REQUIRE(elem == std::nullopt);
}

TEST_CASE("messages buffer get_for existing", "[single-file]") {
  using namespace std::chrono_literals;

  Messages buf;
  std::thread t1{[&buf]() {
    auto elem = buf.get_for(1s);
    REQUIRE(elem.has_value());
    REQUIRE(elem.value()->get_topic() == std::string("/sample/topic"));
    REQUIRE_THAT(elem.value()->get_payload().c_str(),
                 Catch::Matchers::Equals("sample payload"));
  }};
  buf.put(mqtt::make_message("/sample/topic", "sample payload"));
  t1.join();
}

TEST_CASE("messages buffer get_for existing multi", "[single-file]") {
  using namespace std::chrono_literals;

  constexpr int ROUNDS = 100;

  Messages buf;
  std::thread t1{[&buf]() {
    for (int i = 0; i < ROUNDS; i++) {
      auto elem = buf.get_for(1s);
      REQUIRE(elem.has_value());

      REQUIRE_THAT(elem.value()->get_topic(),
                   Catch::Matchers::Equals("/sample/topic"));

      std::stringstream payload;
      payload << "round ";
      payload << i;
      REQUIRE_THAT(elem.value()->get_payload().c_str(),
                   Catch::Matchers::Equals(payload.str().c_str()));
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
