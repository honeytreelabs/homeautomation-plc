#pragma once

#include <common.hpp>
#include <gv.hpp>

#include <list>
#include <memory>

namespace HomeAutomation {
namespace Runtime {

// Programs shall only operate on GV memory
class Program {
public:
  virtual void init(std::shared_ptr<HomeAutomation::GV> gv) = 0;
  virtual void execute(std::shared_ptr<HomeAutomation::GV> gv,
                       HomeAutomation::TimeStamp now) = 0;

protected:
  virtual ~Program() {}
};
using Programs = std::list<std::shared_ptr<Program>>;

std::shared_ptr<HomeAutomation::Runtime::Program>
createCppProgram(std::string const &name);

} // namespace Runtime
} // namespace HomeAutomation
