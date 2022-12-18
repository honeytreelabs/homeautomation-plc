#include <gv_factory.hpp>
#include <runtime_factory.hpp>

#include <stdexcept>
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
      init_val: false
    two:
      init_val: false
    three:
      init_val: true
    four:
      init_val: true
  outputs:
    five:
      init_val: false
    six:
      init_val: false
    seven:
      init_val: true
    eight:
      init_val: true
    nine:
      init_val: true
tasks: []
mqtt: {}
)";

  // referenced variables do not exist
  REQUIRE_THROWS_AS(HomeAutomation::Runtime::RuntimeFactory::fromString(yaml),
                    std::invalid_argument);
}

class CountProgram : public HomeAutomation::Scheduler::Program {
public:
  void execute(HomeAutomation::TimeStamp now) override { cnt++; }
  int cnt = 0;
};

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

  auto testProgram = std::make_shared<CountProgram>();

  runtime->Scheduler()->getTask("main")->addProgram(testProgram);

  runtime->start([&quit_cond]() -> bool { return quit_cond; });

  std::this_thread::sleep_for(100ms);
  quit_cond = true;

  REQUIRE(runtime->wait() == EXIT_SUCCESS);
  REQUIRE(testProgram->cnt > 0);
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

  std::shared_ptr<HomeAutomation::Runtime::Runtime> runtime;
  REQUIRE_NOTHROW([&runtime]() {
    std::string yaml = R"(---
global_vars:
  inputs:
    anything:
      init_val: true
  outputs:
    ground_office_light:
      init_val: true
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
              4: anything
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
    runtime = RuntimeFactory::fromString(yaml);
  }());

  REQUIRE(std::get<bool>(runtime->GV()->inputs["sr_raff_down"]) == false);
  REQUIRE(std::get<bool>(runtime->GV()->inputs["sr_raff_up"]) == false);
  REQUIRE(std::get<bool>(runtime->GV()->inputs["kizi_2_raff_up"]) == false);
  REQUIRE(std::get<bool>(runtime->GV()->inputs["kizi_2_raff_down"]) == false);
  REQUIRE(std::get<bool>(runtime->GV()->inputs["anything"]) == true);

  REQUIRE(std::get<bool>(runtime->GV()->outputs["sr_raff_up"]) == false);
  REQUIRE(std::get<bool>(runtime->GV()->outputs["sr_raff_down"]) == false);
  REQUIRE(std::get<bool>(runtime->GV()->outputs["kizi_2_raff_up"]) == false);
  REQUIRE(std::get<bool>(runtime->GV()->outputs["kizi_2_raff_down"]) == false);
  REQUIRE(std::get<bool>(runtime->GV()->outputs["ground_office_light"]) ==
          true);
}
