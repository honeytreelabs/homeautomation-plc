#pragma once

#include <common.hpp>
#include <program.hpp>
#include <task_io.hpp>

#include <spdlog/spdlog.h>

#include <chrono>
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>

namespace HomeAutomation {
namespace Runtime {

using milliseconds = std::chrono::duration<double, std::milli>;

using QuitCb = std::function<bool()>;

class Task final {
public:
  Task(std::shared_ptr<TaskIOLogic> taskIOLogic, milliseconds interval)
      : taskIOLogic{taskIOLogic}, interval{interval} {}
  void addProgram(std::shared_ptr<HomeAutomation::Runtime::Program> program) {
    programs.push_back(program);
  }
  void executePrograms(
      TimeStamp now = std::chrono::high_resolution_clock::now()) const {
    for (auto const &program : programs) {
      program->execute(now);
    }
  }
  std::shared_ptr<TaskIOLogic> getTaskIOLogic() const { return taskIOLogic; }
  milliseconds getInterval() const { return interval; }

private:
  std::shared_ptr<TaskIOLogic> taskIOLogic;
  Programs programs;
  milliseconds interval;
};

class Scheduler final {
public:
  Scheduler() = default;
  Scheduler(Scheduler &&) = default;

  void installTask(std::string const &name,
                   std::shared_ptr<TaskIOLogic> taskIOLogic,
                   milliseconds interval) {
    if (tasks.find(name) != tasks.end()) {
      throw std::invalid_argument("task with given name already exists");
    }
    tasks.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                  std::forward_as_tuple(taskIOLogic, interval));
  }

  void addProgram(std::string const &name, std::shared_ptr<Program> program) {
    auto const &it = tasks.find(name);
    if (it == tasks.end()) {
      throw std::invalid_argument("task with given name does not exist");
    }
    auto &task = it->second;
    task.addProgram(program);
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
    task.getTaskIOLogic()->init();
    while (!quitCb()) {
      spdlog::debug("Task::tick()");
      task.getTaskIOLogic()->before();
      task.executePrograms();
      task.getTaskIOLogic()->after();

      // TODO this should account for the actual program execution period
      std::this_thread::sleep_for(task.getInterval());
    }
    task.getTaskIOLogic()->shutdown();
  }

  Scheduler(Scheduler &) = delete;
  Scheduler &operator=(Scheduler &) = delete;
  Scheduler &operator=(Scheduler &&) = delete;

  std::map<std::string, Task> tasks;
  std::list<std::thread> threads;
};

} // namespace Runtime
} // namespace HomeAutomation
