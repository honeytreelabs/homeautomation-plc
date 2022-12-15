#include <runtime_impl.hpp>

#include <yaml-cpp/yaml.h>

namespace HomeAutomation {
namespace Runtime {

static RuntimeImpl *initRuntime(YAML::Node const &rootNode) {
  auto gv = GVFactory::generateGVs(rootNode["global_vars"]);
  auto mqttClients = MQTTFactory::generateClients(rootNode["mqtt"]);
  auto scheduler =
      SchedulerFactory::createScheduler(rootNode["tasks"], gv, mqttClients);

  return new RuntimeImpl{std::move(gv), std::move(mqttClients), scheduler};
}

std::shared_ptr<Runtime> RuntimeFactory::fromString(std::string const &str) {
  YAML::Node rootNode = YAML::Load(str);

  return std::shared_ptr<Runtime>(initRuntime(rootNode));
}

std::shared_ptr<Runtime>
RuntimeFactory::fromFile(std::filesystem::path const &path) {
  YAML::Node rootNode = YAML::LoadFile(path);

  return std::shared_ptr<Runtime>(initRuntime(rootNode));
}

} // namespace Runtime
} // namespace HomeAutomation
