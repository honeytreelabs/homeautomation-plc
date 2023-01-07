// PLC runtime
#include <program.hpp>

#include <sstream>

namespace HomeAutomation {
namespace Runtime {

std::shared_ptr<HomeAutomation::Runtime::Program>
createCppProgram(std::string const &name) {
  std::stringstream s;
  s << "unknown program named " << name << " requested";
  throw std::invalid_argument(s.str());
}

} // namespace Runtime
} // namespace HomeAutomation
