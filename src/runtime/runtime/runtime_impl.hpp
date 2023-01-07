#pragma once

#include <runtime.hpp>
#include <scheduler.hpp>

namespace HomeAutomation {
namespace Runtime {

class RuntimeImpl : public Runtime {
public:
  RuntimeImpl(std::shared_ptr<HomeAutomation::GV> gv,
              std::shared_ptr<HomeAutomation::Runtime::Scheduler> scheduler)
      : gv{gv}, scheduler{scheduler} {}

public:
  std::shared_ptr<HomeAutomation::GV> GV() override { return gv; }

  std::shared_ptr<HomeAutomation::Runtime::Scheduler> Scheduler() override {
    return scheduler;
  }

  void start(HomeAutomation::Runtime::QuitCb quitCb) override {
    scheduler->start(gv, quitCb);
  }

  int wait() override {
    auto result = scheduler->wait();
    return result;
  }

private:
  std::shared_ptr<HomeAutomation::GV> gv;
  std::shared_ptr<HomeAutomation::Runtime::Scheduler> scheduler;
};

} // namespace Runtime
} // namespace HomeAutomation
