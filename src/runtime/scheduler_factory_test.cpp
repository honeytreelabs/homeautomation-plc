#include <gv_factory.hpp>
#include <scheduler_factory.hpp>

#include <stdexcept>
#include <utility>
#include <yaml-cpp/yaml.h>

#include <catch2/catch_all.hpp>

using namespace HomeAutomation::Runtime;

TEST_CASE("empty", "[single-file]") {
  std::string yaml = R"(---
tasks: []
)";

  auto const &rootNode = YAML::Load(yaml);
  HomeAutomation::GV gv;
  HomeAutomation::Runtime::MQTTClients mqttClients{};

  REQUIRE_NOTHROW(
      SchedulerFactory::createScheduler(rootNode["tasks"], gv, mqttClients));
}

TEST_CASE("one task", "[single-file]") {
  using namespace std::chrono_literals;

  std::string yaml = R"(---
tasks:
  - name: main
    interval: 25000
)";

  auto const &rootNode = YAML::Load(yaml);

  HomeAutomation::GV gv;
  HomeAutomation::Runtime::MQTTClients mqttClients{};
  auto scheduler =
      SchedulerFactory::createScheduler(rootNode["tasks"], gv, mqttClients);

  class : public HomeAutomation::Scheduler::Program {
    void execute(HomeAutomation::TimeStamp now) {}
  } program;
  REQUIRE_NOTHROW(scheduler->getTask("main")->addProgram(&program));
}

TEST_CASE("task referencing not-existing mqtt client", "[single-file]") {
  using namespace std::chrono_literals;

  std::string yaml = R"(---
tasks:
  - name: main
    interval: 25000
    mqtt: doesnotexist
)";

  auto const &rootNode = YAML::Load(yaml);

  HomeAutomation::GV gv;
  HomeAutomation::Runtime::MQTTClients mqttClients{};
  REQUIRE_THROWS_AS(
      SchedulerFactory::createScheduler(rootNode["tasks"], gv, mqttClients),
      std::invalid_argument);
}
