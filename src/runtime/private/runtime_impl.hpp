#pragma once

#include <gv_factory.hpp>
#include <mqtt_factory.hpp>
#include <runtime_factory.hpp>
#include <scheduler_factory.hpp>
#include <scheduler_impl.hpp>

namespace HomeAutomation {
namespace Runtime {

class RuntimeImpl : public Runtime {
public:
  RuntimeImpl(HomeAutomation::GV &&gv, MQTTClients &&mqttClients,
              std::shared_ptr<HomeAutomation::Runtime::SchedulerImpl> scheduler)
      : gv{std::move(gv)},
        mqttClients{std::move(mqttClients)}, scheduler{scheduler} {}

public:
  HomeAutomation::GV *GV() override { return &gv; }

  HomeAutomation::Runtime::Scheduler *Scheduler() override {
    return scheduler.get();
  }

  void start(HomeAutomation::Scheduler::QuitCb quitCb) override {
    for (auto &client : mqttClients) {
      client.second.connect();
    }
    scheduler->start(quitCb);
  }

  int wait() override {
    auto result = scheduler->wait();
    for (auto &client : mqttClients) {
      client.second.disconnect();
    }
    return result;
  }

private:
  HomeAutomation::GV gv;
  std::shared_ptr<HomeAutomation::Runtime::SchedulerImpl> scheduler;
  MQTTClients mqttClients;
};

} // namespace Runtime
} // namespace HomeAutomation
