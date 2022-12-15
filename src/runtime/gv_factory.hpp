#pragma once

#include <gv.hpp>

#include <yaml-cpp/yaml.h>

namespace HomeAutomation {

namespace Runtime {

class GVFactory {
public:
  static HomeAutomation::GV generateGVs(YAML::Node const &gvNode);

private:
  GVFactory() = delete;
};

} // namespace Runtime
} // namespace HomeAutomation
