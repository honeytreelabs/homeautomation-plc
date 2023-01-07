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
      std::shared_ptr<HomeAutomation::Runtime::TaskIOLogicComposite> ioLogic,
      std::shared_ptr<HomeAutomation::GV> gv);
};

} // namespace Runtime
} // namespace HomeAutomation
