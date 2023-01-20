#include <program_factory.hpp>
#include <scheduler.hpp>

#include <stdexcept>
#include <yaml-cpp/yaml.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <sstream>

class SampleProgram final : public HomeAutomation::Runtime::Program {
public:
  SampleProgram() = default;
  void init(HomeAutomation::GV *gv) override { (void)gv; }
  void execute(HomeAutomation::GV *gv, HomeAutomation::TimeStamp now) override {
    (void)gv;
    (void)now;
  }
};

namespace HomeAutomation {
namespace Runtime {

std::shared_ptr<HomeAutomation::Runtime::Program>
createCppProgram(std::string const &name) {
  if (name == "SampleProgram") {
    return std::make_shared<SampleProgram>();
  }
  std::stringstream s;
  s << "unknown program named " << name << " requested";
  throw std::invalid_argument(s.str());
}

} // namespace Runtime
} // namespace HomeAutomation

TEST_CASE("program factory: initialize C++ programs") {
  using namespace std::chrono_literals;

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
  auto task = HomeAutomation::Runtime::Task{"TestTask", taskIOLogic, 500 * 1ms};
  REQUIRE_NOTHROW(HomeAutomation::Runtime::ProgramFactory::installPrograms(
      &task, rootNode["programs"]));
}

TEST_CASE("program factory: initialize undefined C++ programs") {
  using namespace std::chrono_literals;

  auto const &rootNode = YAML::Load(R"(---
programs:
  - name: SampleProgram
    type: C++
  - name: UnknownProgramCausingTrouble
    type: C++
)");

  auto taskIOLogic =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
  auto task = HomeAutomation::Runtime::Task{"TestTask", taskIOLogic, 500 * 1ms};
  REQUIRE_THROWS_AS(HomeAutomation::Runtime::ProgramFactory::installPrograms(
                        &task, rootNode["programs"]),
                    std::invalid_argument);
}

TEST_CASE("program factory: initialize Lua programs") {
  using namespace std::chrono_literals;

  auto const &rootNode = YAML::Load(R"(---
programs:
  - name: First
    type: Lua
    script_path: /opt/homeautomation/first.lua
  - name: Second
    type: Lua
    script_path: /opt/homeautomation/second.lua
  - name: Third
    type: Lua
    script_path: /opt/homeautomation/third.lua
)");

  auto taskIOLogic =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
  auto task = HomeAutomation::Runtime::Task{"TestTask", taskIOLogic, 500 * 1ms};
  REQUIRE_THROWS_AS(HomeAutomation::Runtime::ProgramFactory::installPrograms(
                        &task, rootNode["programs"]),
                    std::invalid_argument);
}

TEST_CASE(
    "program factory: initialize Lua programs with undefined script path") {
  using namespace std::chrono_literals;

  auto const &rootNode = YAML::Load(R"(---
programs:
  - name: First
    type: Lua
    # script_path undefined
  - name: Second
    type: Lua
    script_path: /opt/homeautomation/second.lua
  - name: Third
    type: Lua
    script_path: /opt/homeautomation/third.lua
)");

  auto taskIOLogic =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
  auto task = HomeAutomation::Runtime::Task{"TestTask", taskIOLogic, 500 * 1ms};
  REQUIRE_THROWS_AS(HomeAutomation::Runtime::ProgramFactory::installPrograms(
                        &task, rootNode["programs"]),
                    std::invalid_argument);
}

TEST_CASE("program factory: execute Lua program") {
  using namespace std::chrono_literals;

  HomeAutomation::GV gv{};

  gv.inputs["foo"] = true;
  gv.outputs["bar"] = 42;

  auto const &programsRootNode = YAML::Load(R"(---
programs:
  - name: First
    type: Lua
    script_path: test/test_program.lua
)");

  auto taskIOLogic =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
  auto task = HomeAutomation::Runtime::Task{"TestTask", taskIOLogic, 500 * 1ms};
  REQUIRE_NOTHROW(HomeAutomation::Runtime::ProgramFactory::installPrograms(
      &task, programsRootNode["programs"]));

  task.initPrograms(&gv);

  task.executePrograms(&gv);
  task.executePrograms(&gv);
  task.executePrograms(&gv);

  REQUIRE(std::get<int>(gv.outputs["bar"]) == 45);
}

TEST_CASE("program factory: execute Lua program with library components") {
  using namespace std::chrono_literals;

  HomeAutomation::GV gv{};

  gv.inputs["input_1"] = false;
  gv.outputs["output_1"] = false;

  auto const &programsRootNode = YAML::Load(R"(---
programs:
  - name: First
    type: Lua
    script_path: test/test_program_with_library.lua
)");

  auto taskIOLogic =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
  auto task = HomeAutomation::Runtime::Task{"TestTask", taskIOLogic, 500 * 1ms};
  REQUIRE_NOTHROW(HomeAutomation::Runtime::ProgramFactory::installPrograms(
      &task, programsRootNode["programs"]));

  task.initPrograms(&gv);
  REQUIRE(std::get<bool>(gv.outputs["output_1"]) == false);

  task.executePrograms(&gv);
  REQUIRE(std::get<bool>(gv.outputs["output_1"]) == false);

  gv.inputs["input_1"] = true;
  task.executePrograms(&gv);
  REQUIRE(std::get<bool>(gv.outputs["output_1"]) == true);

  gv.inputs["input_1"] = false;
  task.executePrograms(&gv);
  REQUIRE(std::get<bool>(gv.outputs["output_1"]) == true);

  gv.inputs["input_1"] = false;
  task.executePrograms(&gv);
  REQUIRE(std::get<bool>(gv.outputs["output_1"]) == true);

  gv.inputs["input_1"] = true;
  task.executePrograms(&gv);
  REQUIRE(std::get<bool>(gv.outputs["output_1"]) == false);

  gv.inputs["input_1"] = false;
  task.executePrograms(&gv);
  REQUIRE(std::get<bool>(gv.outputs["output_1"]) == false);
}

TEST_CASE("program factory: execute Lua program with blind") {
  using namespace std::chrono_literals;

  HomeAutomation::GV gv{};

  gv.inputs["input_up"] = false;
  gv.inputs["input_down"] = false;
  gv.outputs["output_up"] = false;
  gv.outputs["output_down"] = false;

  auto const &programsRootNode = YAML::Load(R"(---
programs:
  - name: First
    type: Lua
    script_path: test/test_program_with_blind.lua
)");

  auto taskIOLogic =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
  auto task = HomeAutomation::Runtime::Task{"TestTask", taskIOLogic, 500 * 1ms};
  REQUIRE_NOTHROW(HomeAutomation::Runtime::ProgramFactory::installPrograms(
      &task, programsRootNode["programs"]));

  auto start = std::chrono::steady_clock::now();

  task.initPrograms(&gv);

  gv.inputs["input_up"] = false;
  gv.inputs["input_down"] = false;
  REQUIRE_NOTHROW(task.executePrograms(&gv, start));
  REQUIRE(std::get<bool>(gv.outputs["output_up"]) == false);
  REQUIRE(std::get<bool>(gv.outputs["output_down"]) == false);

  gv.inputs["input_up"] = true;
  gv.inputs["input_down"] = false;
  REQUIRE_NOTHROW(task.executePrograms(&gv, start + 600ms));
  REQUIRE(std::get<bool>(gv.outputs["output_up"]) == true);
  REQUIRE(std::get<bool>(gv.outputs["output_down"]) == false);
}

TEST_CASE("program factory: execute Lua program directly from YAML") {
  using namespace std::chrono_literals;

  HomeAutomation::GV gv{};

  gv.inputs["foo"] = true;
  gv.outputs["bar"] = 42;

  auto const &programsRootNode = YAML::Load(R"(---
programs:
  - name: First
    type: Lua
    script: |
      function Init(gv) end
      function Cycle(gv, now)
        if gv.inputs.foo then gv.outputs.bar = gv.outputs.bar + 1 end
      end
)");

  auto taskIOLogic =
      std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
  auto task = HomeAutomation::Runtime::Task{"TestTask", taskIOLogic, 500 * 1ms};
  REQUIRE_NOTHROW(HomeAutomation::Runtime::ProgramFactory::installPrograms(
      &task, programsRootNode["programs"]));

  task.initPrograms(&gv);

  task.executePrograms(&gv);
  task.executePrograms(&gv);
  task.executePrograms(&gv);

  REQUIRE(std::get<int>(gv.outputs["bar"]) == 45);
}
