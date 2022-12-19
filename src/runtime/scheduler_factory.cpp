#include <io_factory.hpp>
#include <program_factory.hpp>
#include <scheduler_factory.hpp>
#include <scheduler_impl.hpp>

#include <chrono>
#include <stdexcept>

namespace HomeAutomation {
namespace Runtime {

std::shared_ptr<SchedulerImpl>
SchedulerFactory::createScheduler(YAML::Node const &schedulerNode,
                                  HomeAutomation::GV *gv,
                                  MQTTClients *mqttClients) {
  using namespace std::chrono_literals;

  auto scheduler = std::make_shared<HomeAutomation::Runtime::SchedulerImpl>();

  for (YAML::const_iterator it = schedulerNode.begin();
       it != schedulerNode.end(); ++it) {
    auto const &taskNode = *it;

    // MQTT
    HomeAutomation::Components::MQTT::ClientPaho *mqttClient;
    auto const &mqttNode = taskNode["mqtt"];
    if (mqttNode.IsDefined()) {
      auto const &mqttClientIt = mqttClients->find(mqttNode.as<std::string>());
      if (mqttClientIt == mqttClients->end()) {
        throw std::invalid_argument("given mqtt client could not be found");
      }
      mqttClient = &mqttClientIt->second;
    }

    // IO
    auto taskIOLogic = std::make_shared<TaskIOLogicImpl>();
    auto const &ioNode = taskNode["io"];
    IOFactory::createIOs(ioNode, taskIOLogic, gv, mqttClients);

    // install task
    auto const &taskName = taskNode["name"].as<std::string>();
    spdlog::info("Installing task {}", taskName);
    scheduler->installTask(taskName, taskIOLogic,
                           taskNode["interval"].as<int>() * 1us, mqttClient);

    // TODO scheduler->installTask should return newly created task
    auto task = scheduler->getTask(taskName);
    ProgramFactory::installPrograms(task, gv, mqttClient, taskNode["programs"]);
  }

  return scheduler;
}

} // namespace Runtime
} // namespace HomeAutomation
