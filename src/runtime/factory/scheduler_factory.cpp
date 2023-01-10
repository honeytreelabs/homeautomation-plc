#include <io_factory.hpp>
#include <program_factory.hpp>
#include <scheduler_factory.hpp>

#include <chrono>
#include <stdexcept>

namespace HomeAutomation {
namespace Runtime {

void SchedulerFactory::initializeScheduler(YAML::Node const &schedulerNode,
                                           HomeAutomation::GV *gv,
                                           Scheduler *scheduler) {
  using namespace std::chrono_literals;

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
}

} // namespace Runtime
} // namespace HomeAutomation
