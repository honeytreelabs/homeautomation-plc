#pragma once

#include <scheduler.hpp>

#include <yaml-cpp/yaml.h>

namespace HomeAutomation {
namespace Runtime {

class SchedulerFactory {
public:
  static void
  initializeScheduler(YAML::Node const &schedulerNode, HomeAutomation::GV *gv,
                      HomeAutomation::Runtime::Scheduler *scheduler);

private:
  SchedulerFactory() = delete;
};

} // namespace Runtime
} // namespace HomeAutomation
