#pragma once

#include <map>
#include <string>
#include <variant>

namespace HomeAutomation {

using VarValue = std::variant<bool, int>;
using GvSegment = std::map<std::string, VarValue>;
struct GV {
  GvSegment inputs;
  GvSegment outputs;
};

} // namespace HomeAutomation
