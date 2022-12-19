#include <program_factory.hpp>

namespace HomeAutomation {
namespace Runtime {
void ProgramFactory::installPrograms(
    HomeAutomation::Runtime::Task *task, HomeAutomation::GV *gv,
    HomeAutomation::Components::MQTT::ClientPaho *mqtt,
    YAML::Node const &programsNode) {
  if (!programsNode.IsDefined()) {
    spdlog::info("No programs defined.");
    return;
  }
  for (YAML::const_iterator it = programsNode.begin(); it != programsNode.end();
       ++it) {
    auto const &programNode = *it;
    auto const &programType = programNode["type"].as<std::string>();
    auto const &programName = programNode["name"].as<std::string>();
    std::string const cppProgramTypeName = "C++";
    if (programType == cppProgramTypeName) {
      spdlog::info("Creating {} program {}", cppProgramTypeName, programName);
      auto program = createCppProgram(programName, gv, mqtt);
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
