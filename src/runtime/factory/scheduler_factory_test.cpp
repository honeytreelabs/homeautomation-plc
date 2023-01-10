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
  HomeAutomation::GV gv{};
  HomeAutomation::Runtime::Scheduler scheduler{};

  REQUIRE_NOTHROW(SchedulerFactory::initializeScheduler(rootNode["tasks"], &gv,
                                                        &scheduler));
}

class NullProgram : public HomeAutomation::Runtime::Program {
  void init(HomeAutomation::GV *gv) override { (void)gv; }
  void execute(HomeAutomation::GV *gv, HomeAutomation::TimeStamp now) override {
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

  HomeAutomation::GV gv{};
  HomeAutomation::Runtime::Scheduler scheduler{};

  SchedulerFactory::initializeScheduler(rootNode["tasks"], &gv, &scheduler);

  auto program = std::make_shared<NullProgram>();
  REQUIRE_NOTHROW(scheduler.getTask("main")->addProgram(program));
}
