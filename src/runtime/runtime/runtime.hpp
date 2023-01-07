#pragma once

#include <gv.hpp>
#include <scheduler.hpp>

namespace HomeAutomation {
namespace Runtime {

class Runtime {
public:
  Runtime() = default;
  virtual ~Runtime() = default;

  virtual void start(HomeAutomation::Runtime::QuitCb quitCb) = 0;
  virtual int wait() = 0;

  virtual std::shared_ptr<HomeAutomation::GV> GV() = 0;
  virtual std::shared_ptr<HomeAutomation::Runtime::Scheduler> Scheduler() = 0;
};

} // namespace Runtime
} // namespace HomeAutomation
