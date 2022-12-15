#pragma once

#include <common.hpp>

#include <spdlog/spdlog.h>

#include <chrono>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>

namespace HomeAutomation {
namespace Scheduler {

using milliseconds = std::chrono::duration<double, std::milli>;

// Programs shall only operate on GV memory
class Program {
public:
  virtual ~Program() {}
  virtual void execute(TimeStamp now) = 0;
};

using QuitCb = std::function<bool()>;

class TaskIOLogic {
public:
  TaskIOLogic() = default;
  virtual ~TaskIOLogic() = default;

  virtual void init() = 0;
  virtual void shutdown() = 0;
  virtual void before() = 0;
  virtual void after() = 0;
};

using Programs = std::list<Program *>;
struct Task {
  std::shared_ptr<TaskIOLogic> taskLogic;
  std::list<Program *> programs;
  milliseconds interval;
};

class Scheduler final {
public:
  Scheduler() = default;
  Scheduler(Scheduler &&) = default;

  void installTask(std::string const &name,
                   std::shared_ptr<TaskIOLogic> taskLogic,
                   milliseconds interval) {
    if (tasks.find(name) != tasks.end()) {
      throw std::invalid_argument("task with given name already exists");
    }
    tasks[name] = {.taskLogic = taskLogic, .programs{}, .interval = interval};
  }

  void addProgram(std::string const &name, Program *program) {
    auto const &it = tasks.find(name);
    if (it == tasks.end()) {
      throw std::invalid_argument("task with given name does not exist");
    }
    auto &taskInfo = it->second;
    taskInfo.programs.push_back(program);
  }

  Task *getTask(std::string const &name) {
    auto const &it = tasks.find(name);
    if (it == tasks.end()) {
      throw std::invalid_argument("task with given name does not exist");
    }
    auto &taskInfo = it->second;
    return &taskInfo;
  }

  void start(QuitCb quitCb) {
    for (auto &task : tasks) {
      threads.emplace_back(Scheduler::thrdFun, std::cref(task.second), quitCb);
    }
  }

  int wait() {
    for (auto &thread : threads) {
      thread.join();
    }
    return EXIT_SUCCESS;
  }

private:
  static void thrdFun(Task const &taskInfo, QuitCb quitCb) {
    taskInfo.taskLogic->init();
    while (!quitCb()) {
      spdlog::debug("Task::tick()");
      taskInfo.taskLogic->before();
      for (auto &program : taskInfo.programs) {
        program->execute(std::chrono::high_resolution_clock::now());
      }
      taskInfo.taskLogic->after();

      // TODO this should account for the actual program execution period
      std::this_thread::sleep_for(taskInfo.interval);
    }
    taskInfo.taskLogic->shutdown();
  }

  Scheduler(Scheduler &) = delete;
  Scheduler &operator=(Scheduler &) = delete;
  Scheduler &operator=(Scheduler &&) = delete;

  std::map<std::string, Task> tasks;
  std::list<std::thread> threads;
};

} // namespace Scheduler
} // namespace HomeAutomation
