#include <gv_factory.hpp>
#include <program_factory.hpp>
#include <scheduler_factory.hpp>

#include <yaml-cpp/yaml.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdexcept>
#include <utility>

using namespace HomeAutomation::Runtime;

class NullProgram : public HomeAutomation::Runtime::Program {
  void init(HomeAutomation::GV *gv) override { (void)gv; }
  void execute(HomeAutomation::GV *gv, HomeAutomation::TimeStamp now) override {
    (void)gv;
    (void)now;
  }
};

namespace HomeAutomation::Runtime {
std::shared_ptr<HomeAutomation::Runtime::Program>
createCppProgram(std::string const &name) {
  if (name == "NullProgram") {
    return std::make_shared<NullProgram>();
  }
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

TEST_CASE("scheduler factory: one task") {
  using namespace std::chrono_literals;

  std::string yaml = R"(---
tasks:
  - name: main
    interval: 25000
    programs:
      - name: NullProgram
        type: C++
)";

  auto const &rootNode = YAML::Load(yaml);

  HomeAutomation::GV gv{};
  HomeAutomation::Runtime::Scheduler scheduler{};

  SchedulerFactory::initializeScheduler(rootNode["tasks"], &gv, &scheduler);
}
