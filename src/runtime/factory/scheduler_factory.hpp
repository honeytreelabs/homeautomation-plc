#pragma once

#include <runtime.hpp>
#include <scheduler.hpp>

#include <yaml-cpp/yaml.h>

namespace HomeAutomation {
namespace Runtime {

class SchedulerFactory {
public:
  static std::shared_ptr<HomeAutomation::Scheduler::Scheduler>
  createScheduler(YAML::Node const &schedulerNode, HomeAutomation::GV *gv);

private:
  SchedulerFactory() = delete;
};

} // namespace Runtime
} // namespace HomeAutomation
