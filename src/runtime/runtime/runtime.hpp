#pragma once

#include <gv.hpp>
#include <scheduler.hpp>

namespace HomeAutomation {
namespace Runtime {

class Task {
public:
  Task() = default;
  virtual ~Task() = default;

  virtual void
  addProgram(std::shared_ptr<HomeAutomation::Scheduler::Program> program) = 0;
};

class Scheduler {
public:
  Scheduler() = default;
  virtual ~Scheduler() = default;

  virtual Task *getTask(std::string const &name) = 0;
};

class Runtime {
public:
  Runtime() = default;
  virtual ~Runtime() = default;

  virtual void start(HomeAutomation::Scheduler::QuitCb quitCb) = 0;
  virtual int wait() = 0;

  virtual HomeAutomation::GV *GV() = 0;
  virtual HomeAutomation::Scheduler::Scheduler *Scheduler() = 0;
};

} // namespace Runtime
} // namespace HomeAutomation
