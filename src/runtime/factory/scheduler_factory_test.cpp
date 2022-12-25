#include <gv_factory.hpp>
#include <program_factory.hpp>
#include <scheduler_factory.hpp>

#include <yaml-cpp/yaml.h>

#include <catch2/catch_test_macros.hpp>

#include <stdexcept>
#include <utility>

using namespace HomeAutomation::Runtime;

namespace HomeAutomation::Runtime {
std::shared_ptr<HomeAutomation::Scheduler::CppProgram>
createCppProgram(std::string const &name, HomeAutomation::GV *gv) {
  return std::shared_ptr<HomeAutomation::Scheduler::CppProgram>();
}
} // namespace HomeAutomation::Runtime

TEST_CASE("scheduler factory: empty", "[single-file]") {
  std::string yaml = R"(---
tasks: []
)";

  auto const &rootNode = YAML::Load(yaml);
  HomeAutomation::GV gv;

  REQUIRE_NOTHROW(SchedulerFactory::createScheduler(rootNode["tasks"], &gv));
}

class NullProgram : public HomeAutomation::Scheduler::Program {
  void execute(HomeAutomation::TimeStamp now) {}
};

TEST_CASE("scheduler factory: one task", "[single-file]") {
  using namespace std::chrono_literals;

  std::string yaml = R"(---
tasks:
  - name: main
    interval: 25000
)";

  auto const &rootNode = YAML::Load(yaml);

  HomeAutomation::GV gv;
  auto scheduler = SchedulerFactory::createScheduler(rootNode["tasks"], &gv);

  auto program = std::make_shared<NullProgram>();
  REQUIRE_NOTHROW(scheduler->getTask("main")->addProgram(program));
}
