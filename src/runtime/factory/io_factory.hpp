#pragma once

#include <gv.hpp>
#include <scheduler.hpp>

#include <yaml-cpp/yaml.h>

namespace HomeAutomation {
namespace Runtime {

class IOFactory {
public:
  static void createIOs(
      YAML::Node const &ioNode,
      std::shared_ptr<HomeAutomation::Scheduler::TaskIOLogicComposite> ioLogic,
      HomeAutomation::GV *gv);
};

} // namespace Runtime
} // namespace HomeAutomation
