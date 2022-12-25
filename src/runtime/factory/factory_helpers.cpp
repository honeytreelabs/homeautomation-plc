#include <factory_helpers.hpp>

namespace HomeAutomation {
namespace Runtime {
namespace Helper {

VarValue &createMissingGVBool(HomeAutomation::GvSegment &gvSegment,
                              std::string const &name) {
  gvSegment[name] = false;
  return gvSegment[name];
}

} // namespace Helper
} // namespace Runtime
} // namespace HomeAutomation
