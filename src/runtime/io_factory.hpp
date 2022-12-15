#pragma once

#include <scheduler_impl.hpp>

#include <yaml-cpp/yaml.h>

namespace HomeAutomation {
namespace Runtime {

class IOFactory {
public:
  static void createIOs(YAML::Node const &ioNode,
                        std::shared_ptr<TaskIOLogicImpl> ioLogic,
                        HomeAutomation::GV &gv);
};

} // namespace Runtime
} // namespace HomeAutomation
