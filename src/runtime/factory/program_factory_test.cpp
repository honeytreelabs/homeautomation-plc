#include <program_factory.hpp>
#include <scheduler.hpp>

#include <stdexcept>
#include <yaml-cpp/yaml.h>

#include <catch2/catch_test_macros.hpp>

#include <sstream>

class SampleProgram final : public HomeAutomation::Runtime::CppProgram {
public:
  SampleProgram(HomeAutomation::GV *gv) : CppProgram(gv) {}
  void execute(HomeAutomation::TimeStamp now) override { (void)now; }
};

namespace HomeAutomation {
namespace Runtime {

std::shared_ptr<HomeAutomation::Runtime::CppProgram>
createCppProgram(std::string const &name, HomeAutomation::GV *gv) {
  if (name == "SampleProgram") {
    return std::make_shared<SampleProgram>(gv);
  }
  std::stringstream s;
  s << "unknown program named " << name << " requested";
  throw std::invalid_argument(s.str());
}

} // namespace Runtime
} // namespace HomeAutomation

TEST_CASE("program factory: initialize C++ programs", "[single-file]") {
  using namespace std::chrono_literals;

  HomeAutomation::GV gv;

  auto const &rootNode = YAML::Load(R"(---
programs:
  - name: SampleProgram
    type: C++
  - name: SampleProgram
    type: C++
  - name: SampleProgram
    type: C++
)");

  auto taskIOLogic =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
  auto task = HomeAutomation::Runtime::Task{taskIOLogic, 500 * 1ms};
  REQUIRE_NOTHROW(HomeAutomation::Runtime::ProgramFactory::installPrograms(
      &task, &gv, rootNode["programs"]));
}

TEST_CASE("program factory: initialize undefined C++ programs",
          "[single-file]") {
  using namespace std::chrono_literals;

  HomeAutomation::GV gv;

  auto const &rootNode = YAML::Load(R"(---
programs:
  - name: SampleProgram
    type: C++
  - name: UnknownProgramCausingTrouble
    type: C++
)");

  auto taskIOLogic =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
  auto task = HomeAutomation::Runtime::Task{taskIOLogic, 500 * 1ms};
  REQUIRE_THROWS_AS(HomeAutomation::Runtime::ProgramFactory::installPrograms(
                        &task, &gv, rootNode["programs"]),
                    std::invalid_argument);
}

TEST_CASE("program factory: initialize Lua programs", "[single-file]") {
  using namespace std::chrono_literals;

  HomeAutomation::GV gv;

  auto const &rootNode = YAML::Load(R"(---
programs:
  - name: First
    type: Lua
    script: /opt/homeautomation/first.lua
  - name: Second
    type: Lua
    script: /opt/homeautomation/second.lua
  - name: Third
    type: Lua
    script: /opt/homeautomation/third.lua
)");

  auto taskIOLogic =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
  auto task = HomeAutomation::Runtime::Task{taskIOLogic, 500 * 1ms};
  REQUIRE_NOTHROW(HomeAutomation::Runtime::ProgramFactory::installPrograms(
      &task, &gv, rootNode["programs"]));
}

TEST_CASE("program factory: initialize Lua programs with undefined script path",
          "[single-file]") {
  using namespace std::chrono_literals;

  HomeAutomation::GV gv;

  auto const &rootNode = YAML::Load(R"(---
programs:
  - name: First
    type: Lua
    # script undefined
  - name: Second
    type: Lua
    script: /opt/homeautomation/second.lua
  - name: Third
    type: Lua
    script: /opt/homeautomation/third.lua
)");

  auto taskIOLogic =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
  auto task = HomeAutomation::Runtime::Task{taskIOLogic, 500 * 1ms};
  REQUIRE_THROWS_AS(HomeAutomation::Runtime::ProgramFactory::installPrograms(
                        &task, &gv, rootNode["programs"]),
                    std::invalid_argument);
}

TEST_CASE("program factory: execute Lua program", "[single-file]") {
  using namespace std::chrono_literals;

  HomeAutomation::GV gv;
  gv.inputs["foo"] = true;
  gv.outputs["bar"] = 42;

  auto const &programsRootNode = YAML::Load(R"(---
programs:
  - name: First
    type: Lua
    script: test/test_program.lua
)");

  auto taskIOLogic =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
  auto task = HomeAutomation::Runtime::Task{taskIOLogic, 500 * 1ms};
  REQUIRE_NOTHROW(HomeAutomation::Runtime::ProgramFactory::installPrograms(
      &task, &gv, programsRootNode["programs"]));

  task.executePrograms();
  task.executePrograms();
  task.executePrograms();

  REQUIRE(std::get<int>(gv.outputs["bar"]) == 44);
}

TEST_CASE("program factory: execute Lua program with library components",
          "[single-file]") {
  using namespace std::chrono_literals;

  HomeAutomation::GV gv;
  gv.inputs["input_1"] = false;
  gv.outputs["output_1"] = false;

  auto const &programsRootNode = YAML::Load(R"(---
programs:
  - name: First
    type: Lua
    script: test/test_program_with_library.lua
)");

  auto taskIOLogic =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
  auto task = HomeAutomation::Runtime::Task{taskIOLogic, 500 * 1ms};
  REQUIRE_NOTHROW(HomeAutomation::Runtime::ProgramFactory::installPrograms(
      &task, &gv, programsRootNode["programs"]));

  task.executePrograms();
  REQUIRE(std::get<bool>(gv.outputs["output_1"]) == false);

  task.executePrograms();
  REQUIRE(std::get<bool>(gv.outputs["output_1"]) == false);

  gv.inputs["input_1"] = true;
  task.executePrograms();
  REQUIRE(std::get<bool>(gv.outputs["output_1"]) == true);

  gv.inputs["input_1"] = false;
  task.executePrograms();
  REQUIRE(std::get<bool>(gv.outputs["output_1"]) == true);

  gv.inputs["input_1"] = false;
  task.executePrograms();
  REQUIRE(std::get<bool>(gv.outputs["output_1"]) == true);

  gv.inputs["input_1"] = true;
  task.executePrograms();
  REQUIRE(std::get<bool>(gv.outputs["output_1"]) == false);

  gv.inputs["input_1"] = false;
  task.executePrograms();
  REQUIRE(std::get<bool>(gv.outputs["output_1"]) == false);
}
