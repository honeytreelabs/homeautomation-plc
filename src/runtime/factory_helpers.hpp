#pragma once

#include <yaml-cpp/yaml.h>

#include <sstream>
#include <stdexcept>

namespace HomeAutomation {
namespace Runtime {
namespace Helper {

template <typename retVal>
retVal getRequiredField(YAML::Node const &node, std::string const &fieldName) {
  auto const &subNode = node[fieldName];
  if (!subNode.IsDefined()) {
    std::stringstream s;
    s << "required field " << fieldName << " undefined";
    throw std::invalid_argument(s.str());
  }
  return subNode.as<retVal>();
}

} // namespace Helper
} // namespace Runtime
} // namespace HomeAutomation
