#include <gv_factory.hpp>
#include <program_factory.hpp>
#include <scheduler_factory.hpp>

#include <yaml-cpp/yaml.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdexcept>
#include <utility>

using namespace HomeAutomation::Runtime;

namespace HomeAutomation::Runtime {
std::shared_ptr<HomeAutomation::Runtime::Program>
createCppProgram(std::string const &name) {
  return std::shared_ptr<HomeAutomation::Runtime::Program>();
}
} // namespace HomeAutomation::Runtime

TEST_CASE("scheduler factory: empty") {
  std::string yaml = R"(---
tasks: []
)";

  auto const &rootNode = YAML::Load(yaml);
  auto gv = std::make_shared<HomeAutomation::GV>();

  REQUIRE_NOTHROW(SchedulerFactory::createScheduler(rootNode["tasks"], gv));
}

class NullProgram : public HomeAutomation::Runtime::Program {
  void init(std::shared_ptr<HomeAutomation::GV> gv) override { (void)gv; }
  void execute(std::shared_ptr<HomeAutomation::GV> gv,
               HomeAutomation::TimeStamp now) override {
    (void)gv;
    (void)now;
  }
};

TEST_CASE("scheduler factory: one task") {
  using namespace std::chrono_literals;

  std::string yaml = R"(---
tasks:
  - name: main
    interval: 25000
)";

  auto const &rootNode = YAML::Load(yaml);

  auto gv = std::make_shared<HomeAutomation::GV>();
  auto scheduler = SchedulerFactory::createScheduler(rootNode["tasks"], gv);

  auto program = std::make_shared<NullProgram>();
  REQUIRE_NOTHROW(scheduler->getTask("main")->addProgram(program));
}
