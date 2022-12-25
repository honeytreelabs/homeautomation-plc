#include <gv_factory.hpp>

#include <stdexcept>
#include <variant>

namespace HomeAutomation {
namespace Runtime {

// only variables of type bool are supported
static void initializeSegment(HomeAutomation::GvSegment &segment,
                              YAML::Node const &node) {
  for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
    // GVs must exist
    auto const gvName = it->first.as<std::string>();
    if (segment.find(gvName) == segment.end()) {
      throw std::invalid_argument("referenced variable does not exist");
    }
    if (!std::holds_alternative<bool>(segment[gvName])) {
      throw std::invalid_argument("referenced variable is not of type bool");
    }
    auto const gvVal = it->second["init_val"].as<bool>();
    segment[gvName] = gvVal;
  }
}

void GVFactory::initializeGVs(YAML::Node const &gvNode,
                              HomeAutomation::GV *gv) {
  if (!gvNode.IsDefined()) {
    return;
  }

  initializeSegment(gv->inputs, gvNode["inputs"]);
  initializeSegment(gv->outputs, gvNode["outputs"]);
}

} // namespace Runtime
} // namespace HomeAutomation
