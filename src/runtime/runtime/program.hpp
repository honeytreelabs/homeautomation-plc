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
  virtual void execute(TimeStamp now) = 0;

protected:
  virtual ~Program() {}
};
using Programs = std::list<std::shared_ptr<Program>>;

class CppProgram : public Program {
public:
  CppProgram(HomeAutomation::GV *gv) : gv{gv} {}

protected:
  virtual ~CppProgram() = default;
  HomeAutomation::GV *gv;
};

std::shared_ptr<HomeAutomation::Runtime::CppProgram>
createCppProgram(std::string const &name, HomeAutomation::GV *gv);

} // namespace Runtime
} // namespace HomeAutomation
