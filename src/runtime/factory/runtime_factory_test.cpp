#include <gv_factory.hpp>
#include <mqtt.hpp>
#include <program_factory.hpp>
#include <runtime_factory.hpp>

#include <stdexcept>
#include <yaml-cpp/yaml.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <atomic>

using namespace HomeAutomation::Runtime;

class CountProgram : public HomeAutomation::Runtime::Program {
public:
  CountProgram() = default;
  void init(HomeAutomation::GV *gv) override {}
  void execute(HomeAutomation::GV *gv, HomeAutomation::TimeStamp now) override {
    cnt++;
  }
  int cnt;
};

namespace HomeAutomation::Runtime {
std::shared_ptr<HomeAutomation::Runtime::Program>
createCppProgram(std::string const &name) {
  if (name == "Count") {
    return std::make_shared<CountProgram>();
  }
  return std::shared_ptr<HomeAutomation::Runtime::Program>();
}
} // namespace HomeAutomation::Runtime

TEST_CASE("runtime factory: empty") {
  std::string yaml = R"(---
global_vars: {}
tasks: []
)";

  HomeAutomation::GV gv{};
  HomeAutomation::Runtime::Scheduler scheduler{};

  REQUIRE_NOTHROW(RuntimeFactory::fromString(yaml, &gv, &scheduler));
}

TEST_CASE("runtime factory: instantiate runtime only tasks") {
  std::string yaml = R"(---
global_vars: {}
tasks:
  - name: main
    interval: 25000
)";

  HomeAutomation::GV gv{};
  HomeAutomation::Runtime::Scheduler scheduler{};

  REQUIRE_NOTHROW(RuntimeFactory::fromString(yaml, &gv, &scheduler));
}

TEST_CASE("runtime factory: instantiate runtime from YAML") {
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
)";

  HomeAutomation::GV gv{};
  HomeAutomation::Runtime::Scheduler scheduler{};

  // referenced variables do not exist
  REQUIRE_THROWS_AS(HomeAutomation::Runtime::RuntimeFactory::fromString(
                        yaml, &gv, &scheduler),
                    std::invalid_argument);
}

TEST_CASE("runtime factory: instantiate and execute runtime missing broker") {
  std::string yaml = R"(---
global_vars: {}
tasks:
  - name: main
    interval: 25000
    programs:
      - name: Count
        type: C++
    io:
      - type: mqtt
        client:
          username: someone
          password: secret
          address: tcp://hostdoesnotexist:1883
          client_id: faultyclient
        inputs: {}
        output: {}
)";

  HomeAutomation::GV gv{};
  HomeAutomation::Runtime::Scheduler scheduler{};

  RuntimeFactory::fromString(yaml, &gv, &scheduler);

  REQUIRE_NOTHROW([&gv, &scheduler]() {
    scheduler.start(&gv);
    scheduler.stop();
    scheduler.wait();
  }());
}

TEST_CASE("runtime factory: instantiate runtime with some programs") {
  using namespace std::chrono_literals;

  REQUIRE_NOTHROW([]() {
    std::string yaml = R"(---
tasks:
  - name: main
    interval: 25000  # us
    programs:
      - name: Count
        type: C++
)";
    std::atomic_bool quit_cond = false;
    HomeAutomation::GV gv{};
    HomeAutomation::Runtime::Scheduler scheduler{};

    RuntimeFactory::fromString(yaml, &gv, &scheduler);

    scheduler.start(&gv);

    std::this_thread::sleep_for(100ms);
    quit_cond = true;

    scheduler.stop();
    REQUIRE(scheduler.wait() == EXIT_SUCCESS);
  }());
}

TEST_CASE("runtime factory: instantiate runtime with all features") {
  HomeAutomation::GV gv{};
  HomeAutomation::Runtime::Scheduler scheduler{};

  REQUIRE_NOTHROW([&gv, &scheduler]() {
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
    programs:
      - name: First
        type: C++
      - name: Second
        type: C++
      - name: Third
        type: C++
    io:
      - type: mqtt
        client:
          username: garfield
          password: secret
          address: tcp://localhost:1883
          client_id: test::main
        inputs: {}
        outputs:
          /homeautomation/something: something
      - type: modbus-rtu
        path: /dev/ttyUSB0
        baud: 9600
        data_bit: 8
        parity: N
        stop_bit: 1
        components:
          - type: WP8026ADAM
            slave: 1
            inputs:
              0: one
              1: two
              2: three
          - type: R4S8CRMB
            slave: 1
            outputs:
              0: one
              1: two
              2: three
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
)";
    RuntimeFactory::fromString(yaml, &gv, &scheduler);
  }());

  REQUIRE(std::get<bool>(gv.inputs["sr_raff_down"]) == false);
  REQUIRE(std::get<bool>(gv.inputs["sr_raff_up"]) == false);
  REQUIRE(std::get<bool>(gv.inputs["kizi_2_raff_up"]) == false);
  REQUIRE(std::get<bool>(gv.inputs["kizi_2_raff_down"]) == false);
  REQUIRE(std::get<bool>(gv.inputs["anything"]) == true);

  REQUIRE(std::get<bool>(gv.outputs["sr_raff_up"]) == false);
  REQUIRE(std::get<bool>(gv.outputs["sr_raff_down"]) == false);
  REQUIRE(std::get<bool>(gv.outputs["kizi_2_raff_up"]) == false);
  REQUIRE(std::get<bool>(gv.outputs["kizi_2_raff_down"]) == false);
  REQUIRE(std::get<bool>(gv.outputs["ground_office_light"]) == true);
}
