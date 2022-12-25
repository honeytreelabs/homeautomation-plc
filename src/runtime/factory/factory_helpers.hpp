#pragma once

#include <gv.hpp>

#include <yaml-cpp/yaml.h>

#include <sstream>
#include <stdexcept>

namespace HomeAutomation {
namespace Runtime {
namespace Helper {

VarValue &createMissingGVBool(HomeAutomation::GvSegment &gvSegment,
                              std::string const &name);

template <typename SequenceType, typename IOType>
void insertCopySequenceBool(SequenceType &sequence,
                            HomeAutomation::GvSegment &gvSegment,
                            std::shared_ptr<IOType> io,
                            YAML::Node const &node) {
  for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
    auto const gvName = it->second.as<std::string>();
    uint8_t pin = it->first.as<uint8_t>();

    auto &gvVal = createMissingGVBool(gvSegment, gvName);
    sequence.emplace_back(io, &gvVal, pin);
  }
}

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
