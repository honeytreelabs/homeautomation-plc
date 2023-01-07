#pragma once

#include <program.hpp>

#include <sol/sol.hpp>

#include <filesystem>

namespace HomeAutomation {
namespace Runtime {

class LuaProgram : public Program {
public:
  LuaProgram(std::filesystem::path const &path);
  LuaProgram(std::string const &script);
  void init(std::shared_ptr<HomeAutomation::GV> gv) override;
  void execute(std::shared_ptr<HomeAutomation::GV> gv, TimeStamp now) override;

private:
  static void InitLuaInterpreter(sol::state &lua,
                                 std::function<void()> strategy);
  sol::state lua;
};
} // namespace Runtime
} // namespace HomeAutomation
