#pragma once

#include <gv.hpp>
#include <mqtt.hpp>
#include <scheduler.hpp>

namespace HomeAutomation {
namespace Runtime {

using MQTTClients =
    std::map<std::string, HomeAutomation::Components::MQTT::ClientPaho>;

class Task {
public:
  Task() = default;
  virtual ~Task() = default;

  virtual Components::MQTT::ClientPaho *MQTT() = 0;
  virtual void addProgram(HomeAutomation::Scheduler::Program *program) = 0;
};

class Scheduler {
public:
  Scheduler() = default;
  virtual ~Scheduler() = default;

  virtual Task *getTask(std::string const &name) = 0;
};

class Runtime {
public:
  Runtime() = default;
  virtual ~Runtime() = default;

  virtual void start(HomeAutomation::Scheduler::QuitCb quitCb) = 0;
  virtual int wait() = 0;

  virtual HomeAutomation::GV *GV() = 0;
  virtual HomeAutomation::Runtime::Scheduler *Scheduler() = 0;
};

} // namespace Runtime
} // namespace HomeAutomation
