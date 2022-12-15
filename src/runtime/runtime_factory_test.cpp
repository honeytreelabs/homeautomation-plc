#include <gv_factory.hpp>
#include <runtime_factory.hpp>

#include <yaml-cpp/yaml.h>

#include <catch2/catch_test_macros.hpp>

#include <atomic>

using namespace HomeAutomation::Runtime;

TEST_CASE("empty", "[single-file]") {
  std::string yaml = R"(---
global_vars: {}
tasks: []
mqtt: {}
)";

  REQUIRE_NOTHROW(RuntimeFactory::fromString(yaml));
}

TEST_CASE("instantiate runtime only tasks", "[single-file]") {
  std::string yaml = R"(---
global_vars: {}
tasks:
  - name: main
    interval: 25000
mqtt: {}
)";

  REQUIRE_NOTHROW(RuntimeFactory::fromString(yaml));
}

TEST_CASE("instantiate runtime from YAML", "[single-file]") {
  std::string yaml = R"(---
global_vars:
  inputs:
    one:
      type: bool
      init_val: false
    two:
      type: bool
      init_val: false
    three:
      type: bool
      init_val: true
    four:
      type: bool
      init_val: true
  outputs:
    five:
      type: bool
      init_val: false
    six:
      type: bool
      init_val: false
    seven:
      type: bool
      init_val: true
    eight:
      type: bool
      init_val: true
    nine:
      type: bool
      init_val: true
tasks: []
mqtt: {}
)";

  auto runtime = RuntimeFactory::fromString(yaml);

  REQUIRE(std::get<bool>(runtime->GV()->inputs["one"]) == false);
  REQUIRE(std::get<bool>(runtime->GV()->inputs["two"]) == false);
  REQUIRE(std::get<bool>(runtime->GV()->inputs["three"]) == true);
  REQUIRE(std::get<bool>(runtime->GV()->inputs["four"]) == true);
  REQUIRE(std::get<bool>(runtime->GV()->outputs["five"]) == false);
  REQUIRE(std::get<bool>(runtime->GV()->outputs["six"]) == false);
  REQUIRE(std::get<bool>(runtime->GV()->outputs["seven"]) == true);
  REQUIRE(std::get<bool>(runtime->GV()->outputs["eight"]) == true);
  REQUIRE(std::get<bool>(runtime->GV()->outputs["nine"]) == true);
}

TEST_CASE("instantiate and execute runtime", "[single-file]") {
  using namespace std::chrono_literals;

  std::string yaml = R"(---
global_vars: {}
tasks:
  - name: main
    interval: 25000
mqtt: {}
)";

  std::atomic_bool quit_cond = false;
  auto runtime = RuntimeFactory::fromString(yaml);

  class : public HomeAutomation::Scheduler::Program {
  public:
    void execute(HomeAutomation::TimeStamp now) override { cnt++; }
    int cnt = 0;
  } testProgram;

  runtime->Scheduler()->getTask("main")->addProgram(&testProgram);

  runtime->start([&quit_cond]() -> bool { return quit_cond; });

  std::this_thread::sleep_for(100ms);
  quit_cond = true;

  REQUIRE(runtime->wait() == EXIT_SUCCESS);
  REQUIRE(testProgram.cnt > 0);
}

TEST_CASE("instantiate and execute runtime missing broker", "[single-file]") {
  std::string yaml = R"(---
global_vars: {}
tasks:
  - name: main
    interval: 25000
    mqtt: primary
mqtt:
  primary:
    username: someone
    password: secret
    address: tcp://hostdoesnotexist:1883
    client_id: faultyclient
    topics: []
)";

  auto runtime = RuntimeFactory::fromString(yaml);

  REQUIRE_THROWS_AS(
      [&runtime]() {
        runtime->start([]() -> bool { return false; });
        runtime->wait();
      }(),
      mqtt::exception);
}

TEST_CASE("instantiate runtime with all features", "[single-file]") {
  std::string yaml = R"(---
global_vars:
  inputs:
    sr_raff_up:
      type: bool
      init_val: false
    sr_raff_down:
      type: bool
      init_val: false
    kizi_2_raff_up:
      type: bool
      init_val: false
    kizi_2_raff_down:
      type: bool
      init_val: false
  outputs:
    sr_raff_up:
      type: bool
      init_val: false
    sr_raff_down:
      type: bool
      init_val: false
    kizi_2_raff_up:
      type: bool
      init_val: false
    kizi_2_raff_down:
      type: bool
      init_val: false
    ground_office_light:
      type: bool
      init_val: false
tasks:
  - name: main
    interval: 25000  # us
    mqtt: primary
    io:
      - type: i2c
        bus: /dev/i2c-1
        components:
          3b:  # i2c address
            type: pcf8574
            direction: input
            inputs:
              0: sr_raff_up
              1: sr_raff_down
              2: kizi_2_raff_up
              3: kizi_2_raff_down
          20:  # i2c address
            type: max7311
            direction: output
            outputs:
              0: sr_raff_up
              1: sr_raff_down
              2: kizi_2_raff_up
              3: kizi_2_raff_down
              4: ground_office_light
mqtt:
  primary:
    username: garfield
    password: secret
    address: tcp://localhost:1883
    client_id: myclient
    topics:
      - /homeautomation/ground_office_light
)";

  REQUIRE_NOTHROW(RuntimeFactory::fromString(yaml));
}
