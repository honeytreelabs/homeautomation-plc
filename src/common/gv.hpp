#pragma once

#include <map>
#include <string>
#include <variant>

namespace HomeAutomation {

using VarName = std::string;
using VarValue = std::variant<bool, int>;
using GvSegment = std::map<VarName, VarValue>;
struct GV {
  GvSegment inputs;
  GvSegment outputs;
  GV() = default;
  GV(GV &) = delete;
  GV(GV &&) = default;
  GV &operator=(GV &) = delete;
  GV &operator=(GV &&) = delete;
};

} // namespace HomeAutomation
