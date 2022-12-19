#include <runtime_impl.hpp>

#include <yaml-cpp/yaml.h>

namespace HomeAutomation {
namespace Runtime {

static std::shared_ptr<RuntimeImpl> initRuntime(YAML::Node const &rootNode) {
  // TODO make sure gv and mqttClients survive this functions
  auto gv = std::make_shared<HomeAutomation::GV>();
  auto mqttClients = MQTTFactory::generateClients(rootNode["mqtt"]);
  auto scheduler = SchedulerFactory::createScheduler(
      rootNode["tasks"], gv.get(), mqttClients.get());
  GVFactory::initializeGVs(rootNode["global_vars"], gv.get());

  return std::make_shared<RuntimeImpl>(gv, mqttClients, scheduler);
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
