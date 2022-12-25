#pragma once

#include <runtime.hpp>

#include <yaml-cpp/yaml.h>

namespace HomeAutomation {
namespace Scheduler {
class CppProgram : public Program {
public:
  CppProgram(HomeAutomation::GV *gv) : gv{gv} {}

protected:
  virtual ~CppProgram() = default;
  HomeAutomation::GV *gv;
};
} // namespace Scheduler

namespace Runtime {

std::shared_ptr<HomeAutomation::Scheduler::CppProgram>
createCppProgram(std::string const &name, HomeAutomation::GV *gv);

// TODO Lua and Iec Factories will be provided by the Runtime

class ProgramFactory {
public:
  static void installPrograms(HomeAutomation::Runtime::Task *task,
                              HomeAutomation::GV *gv,
                              YAML::Node const &programsNode);
};

} // namespace Runtime
} // namespace HomeAutomation
