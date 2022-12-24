#include <runtime_impl.hpp>

#include <yaml-cpp/yaml.h>

namespace HomeAutomation {
namespace Runtime {

static std::shared_ptr<RuntimeImpl> initRuntime(YAML::Node const &rootNode) {
  auto gv = std::make_shared<HomeAutomation::GV>();
  auto scheduler =
      SchedulerFactory::createScheduler(rootNode["tasks"], gv.get());
  GVFactory::initializeGVs(rootNode["global_vars"], gv.get());

  return std::make_shared<RuntimeImpl>(gv, scheduler);
}

std::shared_ptr<Runtime> RuntimeFactory::fromString(std::string const &str) {
  YAML::Node rootNode = YAML::Load(str);

  return initRuntime(rootNode);
}

std::shared_ptr<Runtime>
RuntimeFactory::fromFile(std::filesystem::path const &path) {
  YAML::Node rootNode = YAML::LoadFile(path);

  return initRuntime(rootNode);
}

} // namespace Runtime
} // namespace HomeAutomation
