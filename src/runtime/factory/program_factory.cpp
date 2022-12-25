#include <factory_helpers.hpp>
#include <program_factory.hpp>

namespace HomeAutomation {
namespace Runtime {
void ProgramFactory::installPrograms(HomeAutomation::Scheduler::Task *task,
                                     HomeAutomation::GV *gv,
                                     YAML::Node const &programsNode) {
  if (!programsNode.IsDefined()) {
    spdlog::info("No programs defined.");
    return;
  }
  for (YAML::const_iterator it = programsNode.begin(); it != programsNode.end();
       ++it) {
    auto const &programNode = *it;
    auto const programType =
        Helper::getRequiredField<std::string>(programNode, "type");
    auto const programName =
        Helper::getRequiredField<std::string>(programNode, "name");
    std::string const cppProgramTypeName = "C++";
    if (programType == cppProgramTypeName) {
      spdlog::info("Creating {} program {}", cppProgramTypeName, programName);
      auto program = createCppProgram(programName, gv);
      if (program) {
        spdlog::info("Installing {} program {}", cppProgramTypeName,
                     programName);
        task->addProgram(program);
      }
    } else {
      throw std::invalid_argument("unsupported program type given");
    }
  }
}
} // namespace Runtime
} // namespace HomeAutomation
