#pragma once

#include <gv.hpp>

#include <yaml-cpp/yaml.h>

namespace HomeAutomation {

namespace Runtime {

class GVFactory {
public:
  static void initializeGVs(YAML::Node const &gvNode,
                            std::shared_ptr<HomeAutomation::GV> gv);

private:
  GVFactory() = delete;
};

} // namespace Runtime
} // namespace HomeAutomation
