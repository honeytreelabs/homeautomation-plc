#pragma once

#include <list>
#include <mqtt_factory.hpp>
#include <runtime.hpp>
#include <scheduler.hpp>
#include <tuple>
#include <utility>

namespace HomeAutomation {
namespace Runtime {

class TaskIOLogicImpl : public HomeAutomation::Scheduler::TaskIOLogic {
public:
  TaskIOLogicImpl() = default;
  virtual ~TaskIOLogicImpl() = default;

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

  void addIOSystem(
      std::shared_ptr<HomeAutomation::Scheduler::TaskIOLogic> ioSystem) {
    ioSystems.push_back(ioSystem);
  }

private:
  std::list<std::shared_ptr<HomeAutomation::Scheduler::TaskIOLogic>> ioSystems;
};

class TaskImpl : public Task {
public:
  TaskImpl(HomeAutomation::Scheduler::Task &task,
           HomeAutomation::Components::MQTT::ClientPaho *mqttClient)
      : task{task}, mqttClient{mqttClient} {}
  virtual ~TaskImpl() = default;

  Components::MQTT::ClientPaho *MQTT() override { return mqttClient; }

  void addProgram(HomeAutomation::Scheduler::Program *program) override {
    task.programs.push_back(program);
  }

private:
  HomeAutomation::Scheduler::Task &task;
  // MQTT client is optional
  HomeAutomation::Components::MQTT::ClientPaho *mqttClient;
};

class SchedulerImpl final : public Scheduler {
public:
  SchedulerImpl() = default;

  void installTask(std::string const &name,
                   std::shared_ptr<TaskIOLogicImpl> taskLogic,
                   HomeAutomation::Scheduler::milliseconds interval,
                   HomeAutomation::Components::MQTT::ClientPaho *mqttClient) {
    scheduler.installTask(name, taskLogic, interval);
    auto task = scheduler.getTask(name);
    tasks.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                  std::forward_as_tuple(*task, mqttClient));
  }

  Task *getTask(std::string const &name) override {
    auto const &it = tasks.find(name);
    if (it == tasks.end()) {
      return nullptr;
    }
    return &it->second;
  }

  void start(HomeAutomation::Scheduler::QuitCb quitCb) {
    scheduler.start(quitCb);
  }

  int wait() { return scheduler.wait(); }

private:
  HomeAutomation::Scheduler::Scheduler scheduler;
  std::map<std::string, TaskImpl> tasks;
};

} // namespace Runtime
} // namespace HomeAutomation
