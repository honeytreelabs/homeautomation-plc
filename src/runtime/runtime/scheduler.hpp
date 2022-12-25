#pragma once

#include <common.hpp>

#include <spdlog/spdlog.h>

#include <chrono>
#include <functional>
#include <list>
#include <map>
#include <memory>
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

class TaskIOLogicComposite : public TaskIOLogic {
public:
  virtual void init() override {
    for (auto &ioSystem : ioSystems) {
      ioSystem->init();
    }
  }

  virtual void shutdown() override {
    for (auto &ioSystem : ioSystems) {
      ioSystem->shutdown();
    }
  }

  virtual void before() override {
    for (auto &ioSystem : ioSystems) {
      ioSystem->before();
    }
  }

  virtual void after() override {
    for (auto &ioSystem : ioSystems) {
      ioSystem->after();
    }
  }

  void add(std::shared_ptr<TaskIOLogic> ioSystem) {
    ioSystems.push_back(ioSystem);
  }

private:
  std::list<std::shared_ptr<TaskIOLogic>> ioSystems;
};

using Programs = std::list<std::shared_ptr<Program>>;
struct Task {
  std::shared_ptr<TaskIOLogic> taskIOLogic;
  std::list<std::shared_ptr<Program>> programs;
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
    tasks[name] = {.taskIOLogic = taskLogic, .programs{}, .interval = interval};
  }

  void addProgram(std::string const &name, std::shared_ptr<Program> program) {
    auto const &it = tasks.find(name);
    if (it == tasks.end()) {
      throw std::invalid_argument("task with given name does not exist");
    }
    auto &task = it->second;
    task.programs.push_back(program);
  }

  Task *getTask(std::string const &name) {
    auto const &it = tasks.find(name);
    if (it == tasks.end()) {
      throw std::invalid_argument("task with given name does not exist");
    }
    auto &task = it->second;
    return &task;
  }

  void start(QuitCb quitCb) {
    for (auto &task : tasks) {
      threads.emplace_back(Scheduler::taskFun, std::cref(task.second), quitCb);
    }
  }

  int wait() {
    for (auto &thread : threads) {
      thread.join();
    }
    return EXIT_SUCCESS;
  }

private:
  static void taskFun(Task const &task, QuitCb quitCb) {
    task.taskIOLogic->init();
    while (!quitCb()) {
      spdlog::debug("Task::tick()");
      task.taskIOLogic->before();
      for (auto &program : task.programs) {
        program->execute(std::chrono::high_resolution_clock::now());
      }
      task.taskIOLogic->after();

      // TODO this should account for the actual program execution period
      std::this_thread::sleep_for(task.interval);
    }
    task.taskIOLogic->shutdown();
  }

  Scheduler(Scheduler &) = delete;
  Scheduler &operator=(Scheduler &) = delete;
  Scheduler &operator=(Scheduler &&) = delete;

  std::map<std::string, Task> tasks;
  std::list<std::thread> threads;
};

} // namespace Scheduler
} // namespace HomeAutomation
