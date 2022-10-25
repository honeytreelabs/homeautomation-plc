#pragma once

#include <common.hpp>

#include <spdlog/spdlog.h>

#include <chrono>
#include <functional>
#include <list>
#include <optional>
#include <thread>

namespace HomeAutomation {
namespace Logic {

using milliseconds = std::chrono::duration<double, std::milli>;

// Programs shall only operate on GV memory
class Program {
public:
  virtual ~Program() {}
  virtual void execute(TimeStamp now) = 0;
};

using TaskCb = std::optional<std::function<void()>>;
using QuitCb = std::function<bool()>;

struct TaskCbs {
  TaskCb init;
  TaskCb before;
  TaskCb after;
  TaskCb shutdown;
  QuitCb quit;
};

class Task {
public:
  // https://www.internalpointers.com/post/c-rvalue-references-and-move-semantics-beginners
  Task(milliseconds ms, TaskCbs const &&taskCbs)
      : ms(ms), taskCbs(taskCbs), programs{} {}
  void addProgram(Program *program) { programs.push_back(program); }

  void thrdfun() {
    if (taskCbs.init) {
      taskCbs.init.value()();
    }
    while (!taskCbs.quit()) {
      spdlog::debug("Task::tick()");
      if (taskCbs.before) {
        taskCbs.before.value()();
      }
      for (auto program : programs) {
        program->execute(std::chrono::high_resolution_clock::now());
      }
      if (taskCbs.after) {
        taskCbs.after.value()();
      }

      // this should account for the actual program execution period
      std::this_thread::sleep_for(ms);
    }
    if (taskCbs.shutdown) {
      taskCbs.shutdown.value()();
    }
  }

private:
  Task(Task &) = delete;
  Task &operator=(Task &) = delete;
  Task(Task &&) = delete;
  Task &operator=(Task &&) = delete;

  milliseconds ms;
  TaskCbs const taskCbs;
  std::list<Program *> programs;
};

class Scheduler {
public:
  Scheduler() : tasks(), threads() {}

  // priority: https://yeahexp.com/setting-thread-priority-in-c11/
  Task &createTask(milliseconds ms, TaskCbs const &&taskCbs) {
    return tasks.emplace_back(ms, std::move(taskCbs));
  }

  void start() {
    for (auto &task : tasks) {
      threads.emplace_back(&Task::thrdfun, &task);
    }
  }

  int wait() {
    for (auto &thread : threads) {
      thread.join();
    }
    return EXIT_SUCCESS;
  }

private:
  Scheduler(Scheduler &) = delete;
  Scheduler &operator=(Scheduler &) = delete;
  Scheduler(Scheduler &&) = delete;
  Scheduler &operator=(Scheduler &&) = delete;

  std::list<Task> tasks;
  std::list<std::thread> threads;
};

} // namespace Logic
} // namespace HomeAutomation
