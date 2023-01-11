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

class Task final {
public:
  Task(std::shared_ptr<TaskIOLogic> taskIOLogic, milliseconds interval)
      : taskIOLogic{taskIOLogic}, interval{interval} {}
  void addProgram(std::shared_ptr<HomeAutomation::Runtime::Program> program) {
    programs.push_back(program);
  }
  void initPrograms(HomeAutomation::GV *gv) const {
    for (auto const &program : programs) {
      program->init(gv);
    }
  }
  void executePrograms(
      HomeAutomation::GV *gv,
      TimeStamp now = std::chrono::high_resolution_clock::now()) const {
    for (auto const &program : programs) {
      program->execute(gv, now);
    }
  }
  std::shared_ptr<TaskIOLogic> getTaskIOLogic() const { return taskIOLogic; }
  milliseconds getInterval() const { return interval; }

private:
  std::shared_ptr<TaskIOLogic> taskIOLogic;
  Programs programs;
  milliseconds interval;
};

enum SchedulerState : std::uint8_t {
  UNKNOWN,
  RUNNING,
  STOPPING,
  STOPPED,
};

class Scheduler final {
public:
  Scheduler() : state{STOPPED}, tasks{}, threads{} {}

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

  void start(HomeAutomation::GV *gv) {
    state = RUNNING;
    for (auto &[name, task] : tasks) {
      threads.emplace_back(Scheduler::taskFun, std::cref(task), gv, &state);
    }
  }

  void stop() { state = STOPPING; }

  std::uint8_t getState() const { return state; }

  int wait() {
    for (auto &thread : threads) {
      thread.join();
    }
    state = STOPPED;
    return EXIT_SUCCESS;
  }

private:
  static void taskFun(Task const &task, HomeAutomation::GV *gv,
                      std::atomic_uint8_t *state) {
    task.getTaskIOLogic()->init();
    task.initPrograms(gv);
    while (*state == RUNNING) {
      spdlog::debug("Task::tick()");
      task.getTaskIOLogic()->before();
      task.executePrograms(gv);
      task.getTaskIOLogic()->after();

      // TODO this should account for the actual program execution period
      std::this_thread::sleep_for(task.getInterval());
    }
    task.getTaskIOLogic()->shutdown();
  }

  Scheduler(Scheduler &) = delete;
  Scheduler &operator=(Scheduler &) = delete;
  Scheduler &operator=(Scheduler &&) = delete;

  std::atomic_uint8_t state;
  std::map<std::string, Task> tasks;
  std::list<std::thread> threads;
};

} // namespace Runtime
} // namespace HomeAutomation
