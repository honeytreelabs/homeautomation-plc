#include <factory_helpers.hpp>
#include <program_factory.hpp>
#include <program_lua.hpp>

namespace HomeAutomation {
namespace Runtime {

constexpr char const *CPPProgramTypeName = "C++";
constexpr char const *LuaProgramTypeName = "Lua";

void ProgramFactory::installPrograms(HomeAutomation::Runtime::Task *task,
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
    std::shared_ptr<Program> program;
    if (programType == CPPProgramTypeName) {
      spdlog::info("Creating {} program {}", CPPProgramTypeName, programName);
      program = createCppProgram(programName); // provided by actual application
    } else if (programType == LuaProgramTypeName) {
      auto const script =
          Helper::getRequiredField<std::string>(programNode, "script_path");
      program = std::make_shared<LuaProgram>(script);
    } else {
      throw std::invalid_argument("unsupported program type given");
    }
    spdlog::info("Installing {} program {}", programType, programName);
    task->addProgram(program);
  }
}
} // namespace Runtime
} // namespace HomeAutomation
