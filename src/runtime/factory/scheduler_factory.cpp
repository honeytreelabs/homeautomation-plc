#include <io_factory.hpp>
#include <program_factory.hpp>
#include <scheduler_factory.hpp>

#include <chrono>
#include <stdexcept>

namespace HomeAutomation {
namespace Runtime {

std::shared_ptr<HomeAutomation::Runtime::Scheduler>
SchedulerFactory::createScheduler(YAML::Node const &schedulerNode,
                                  std::shared_ptr<HomeAutomation::GV> gv) {
  using namespace std::chrono_literals;

  auto scheduler = std::make_shared<HomeAutomation::Runtime::Scheduler>();

  for (YAML::const_iterator it = schedulerNode.begin();
       it != schedulerNode.end(); ++it) {
    auto const &taskNode = *it;

    // IO
    auto taskIOLogic =
        std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
    auto const &ioNode = taskNode["io"];
    IOFactory::createIOs(ioNode, taskIOLogic, gv);

    // install task
    auto const &taskName = taskNode["name"].as<std::string>();
    spdlog::info("Installing task {}", taskName);
    scheduler->installTask(taskName, taskIOLogic,
                           taskNode["interval"].as<int>() * 1us);

    // TODO scheduler->installTask should return newly created task
    auto task = scheduler->getTask(taskName);
    ProgramFactory::installPrograms(task, taskNode["programs"]);
  }

  return scheduler;
}

} // namespace Runtime
} // namespace HomeAutomation
