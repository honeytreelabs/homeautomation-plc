#pragma once

#include <gv.hpp>
#include <scheduler_impl.hpp>

#include <yaml-cpp/yaml.h>

namespace HomeAutomation {
namespace Runtime {

class ModbusRTUFactory {
public:
  static void createIOs(YAML::Node const &ioNode,
                        std::shared_ptr<TaskIOLogicComposite> ioLogic,
                        HomeAutomation::GV *gv);
};

} // namespace Runtime
} // namespace HomeAutomation
