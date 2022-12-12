#pragma once

#include <common.hpp>

#include <sol/sol.hpp>

namespace HomeAutomation {
namespace Scheduler {

class LuaProgram {
public:
  virtual ~LuaProgram() {}

  virtual void execute(TimeStamp now) {}

private:
  LuaProgram();
};

} // namespace Scheduler
} // namespace HomeAutomation
