#pragma once

#include <common.hpp>

#include <sol/sol.hpp>

#include <filesystem>

namespace HomeAutomation {
namespace Scheduler {

class LuaProgram {
public:
  virtual ~LuaProgram() {}

  virtual void execute(TimeStamp now) {}

private:
  LuaProgram(std::filesystem::path const &script);
};

} // namespace Scheduler
} // namespace HomeAutomation
