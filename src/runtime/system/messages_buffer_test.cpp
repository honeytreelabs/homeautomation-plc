#include <circular_buffer.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <sstream>

using Buffer = HomeAutomation::circular_buffer<int, 128>;

TEST_CASE("messages buffer: empty") {
  Buffer buf;
  REQUIRE(buf.empty() == true);
}

TEST_CASE("messages buffer: get") {
  Buffer buf;
  auto elem = buf.get();
  REQUIRE(elem == std::nullopt);
}

TEST_CASE("messages buffer: get_for empty") {
  using namespace std::chrono_literals;

  Buffer buf;
  auto elem = buf.get_for(100ms);
  REQUIRE(elem == std::nullopt);
}

TEST_CASE("messages buffer: get_for existing") {
  using namespace std::chrono_literals;

  Buffer buf;
  std::thread t1{[&buf]() {
    auto elem = buf.get_for(1s);
    REQUIRE(elem.has_value());
    REQUIRE(elem.value() == 1);
  }};
  buf.put(1);
  t1.join();
}

TEST_CASE("messages buffer: get_for existing multi") {
  using namespace std::chrono_literals;

  constexpr int ROUNDS = 100;

  Buffer buf;
  std::thread t1{[&buf]() {
    for (int i = 0; i < ROUNDS; i++) {
      auto elem = buf.get_for(1s);
      REQUIRE(elem.has_value());

      REQUIRE(elem.value() == i);
    }
  }};
  for (int i = 0; i < ROUNDS; i++) {
    buf.put(i);
  }
  t1.join();
}
