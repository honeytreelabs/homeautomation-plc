#pragma once

#include <runtime.hpp>

#include <yaml-cpp/yaml.h>

namespace HomeAutomation {
namespace Runtime {

class ProgramFactory {
public:
  static void installPrograms(HomeAutomation::Runtime::Task *task,
                              HomeAutomation::GV *gv,
                              YAML::Node const &programsNode);
};

} // namespace Runtime
} // namespace HomeAutomation
