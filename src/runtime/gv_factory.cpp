#include <gv_factory.hpp>

#include <stdexcept>

namespace HomeAutomation {
namespace Runtime {

static void fillSegment(HomeAutomation::GvSegment &segment,
                        YAML::Node const &node) {
  for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
    auto var_type = it->second["type"].as<std::string>();
    if (var_type == "bool") {
      segment.emplace(it->first.as<std::string>(),
                      it->second["init_val"].as<bool>());
    } else if (var_type == "int") {
      segment.emplace(it->first.as<std::string>(),
                      it->second["init_val"].as<int>());
    } else {
      throw std::invalid_argument{"type unsupported"};
    }
  }
}

HomeAutomation::GV GVFactory::generateGVs(YAML::Node const &gvNode) {
  HomeAutomation::GV result{};

  fillSegment(result.inputs, gvNode["inputs"]);
  fillSegment(result.outputs, gvNode["outputs"]);

  return result;
}

} // namespace Runtime
} // namespace HomeAutomation
