#pragma once

#include <list>
#include <memory>

namespace HomeAutomation {
namespace Runtime {

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
  void init() override {
    for (auto &ioSystem : ioSystems) {
      ioSystem->init();
    }
  }

  void shutdown() override {
    for (auto &ioSystem : ioSystems) {
      ioSystem->shutdown();
    }
  }

  void before() override {
    for (auto &ioSystem : ioSystems) {
      ioSystem->before();
    }
  }

  void after() override {
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

} // namespace Runtime
} // namespace HomeAutomation
