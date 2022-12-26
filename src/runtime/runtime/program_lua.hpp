#pragma once

#include <program.hpp>

#include <sol/sol.hpp>

#include <filesystem>

namespace HomeAutomation {
namespace Runtime {

class LuaProgram : public Program {
public:
  LuaProgram(std::filesystem::path const &path, HomeAutomation::GV *gv);
  void execute(TimeStamp now) override;

private:
  void transferOutputsToRuntime();
  void transferGVsToInterpreter(GvSegment &segment,
                                std::string const &inpVarName);

  std::filesystem::path const path;
  HomeAutomation::GV *gv;
  sol::state lua;
};
} // namespace Runtime
} // namespace HomeAutomation
