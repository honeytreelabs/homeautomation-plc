#include <gv_factory.hpp>
#include <runtime_factory.hpp>
#include <scheduler_factory.hpp>

#include <yaml-cpp/yaml.h>

namespace HomeAutomation {
namespace Runtime {

void initRuntime(YAML::Node const &rootNode, HomeAutomation::GV *gv,
                 HomeAutomation::Runtime::Scheduler *scheduler) {
  SchedulerFactory::initializeScheduler(rootNode["tasks"], gv, scheduler);
  GVFactory::initializeGVs(rootNode["global_vars"], gv);
}

void RuntimeFactory::fromString(std::string const &str, HomeAutomation::GV *gv,
                                HomeAutomation::Runtime::Scheduler *scheduler) {
  YAML::Node rootNode = YAML::Load(str);

  return initRuntime(rootNode, gv, scheduler);
}

void RuntimeFactory::fromFile(std::filesystem::path const &path,
                              HomeAutomation::GV *gv,
                              HomeAutomation::Runtime::Scheduler *scheduler) {
  YAML::Node rootNode = YAML::LoadFile(path);

  return initRuntime(rootNode, gv, scheduler);
}

} // namespace Runtime
} // namespace HomeAutomation
