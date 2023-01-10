#pragma once

#include <scheduler.hpp>
#include <yaml-cpp/yaml.h>

namespace HomeAutomation {
namespace Runtime {

class ProgramFactory {
public:
  static void installPrograms(HomeAutomation::Runtime::Task *task,
                              YAML::Node const &programsNode);
};

} // namespace Runtime
} // namespace HomeAutomation
