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
  Task(std::string const &name, std::shared_ptr<TaskIOLogic> taskIOLogic,
       milliseconds interval)
      : name{name}, taskIOLogic{taskIOLogic}, interval{interval} {}
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
  std::string const name;
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

  Task *installTask(std::string const &name,
                    std::shared_ptr<TaskIOLogic> taskIOLogic,
                    milliseconds interval) {
    auto &task = tasks.emplace_back(name, taskIOLogic, interval);
    return &task;
  }

  void start(HomeAutomation::GV *gv) {
    state = RUNNING;
    for (auto &task : tasks) {
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
  std::list<Task> tasks;
  std::list<std::thread> threads;
};

} // namespace Runtime
} // namespace HomeAutomation
