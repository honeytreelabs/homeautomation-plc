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

    auto taskIOLogic =
        std::make_shared<HomeAutomation::Runtime::TaskIOLogicComposite>();
    auto const &ioNode = taskNode["io"];
    IOFactory::createIOs(ioNode, taskIOLogic, gv);

    auto const &taskName = taskNode["name"].as<std::string>();
    spdlog::info("Installing task {}", taskName);
    auto task = scheduler->installTask(taskName, taskIOLogic,
                                       taskNode["interval"].as<int>() * 1us);

    ProgramFactory::installPrograms(task, taskNode["programs"]);
  }
}

} // namespace Runtime
} // namespace HomeAutomation
