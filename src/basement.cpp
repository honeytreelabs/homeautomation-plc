// components
#include <light.hpp>
#include <trigger.hpp>

// PLC runtime
#include <entry.hpp>
#include <gv.hpp>
#include <mqtt.hpp>
#include <scheduler.hpp>

#include <spdlog/spdlog.h>

// execution context (shall run in dedicated thread with given cycle time)
class BasementLogic final : public HomeAutomation::Scheduler::CppProgram {

public:
  BasementLogic(HomeAutomation::GV *gv,
                HomeAutomation::Components::MQTT::ClientPaho *mqtt)
      : HomeAutomation::Scheduler::CppProgram(gv, mqtt) {}

  void execute(HomeAutomation::TimeStamp now) override {
    (void)now;

    if (workshop_1_trigger.execute(std::get<bool>(gv->inputs["workshop_1"]))) {
      gv->outputs["workshop_1"] = workshop_1_light.toggle();
    }
    if (workshop_2_trigger.execute(std::get<bool>(gv->inputs["workshop_2"]))) {
      gv->outputs["workshop_2"] = workshop_2_light.toggle();
    }
    if (room_1_trigger.execute(std::get<bool>(gv->inputs["room_1"]))) {
      gv->outputs["room_1"] = room_1_light.toggle();
    }
    if (room_2_trigger.execute(std::get<bool>(gv->inputs["room_2"]))) {
      gv->outputs["room_2"] = room_2_light.toggle();
    }
    if (room_3_trigger.execute(std::get<bool>(gv->inputs["room_3"]))) {
      gv->outputs["room_3"] = room_3_light.toggle();
    }
    if (room_4_trigger.execute(std::get<bool>(gv->inputs["room_4"]))) {
      gv->outputs["room_4"] = room_4_light.toggle();
    }
    if (room_5_trigger.execute(std::get<bool>(gv->inputs["room_5"]))) {
      gv->outputs["room_5"] = room_5_light.toggle();
    }
    if (room_6_trigger.execute(std::get<bool>(gv->inputs["room_6"]))) {
      gv->outputs["room_6"] = room_6_light.toggle();
    }
  }

private:
  // logic blocks
  HomeAutomation::Components::R_TRIG workshop_1_trigger;
  HomeAutomation::Components::R_TRIG workshop_2_trigger;
  HomeAutomation::Components::R_TRIG room_1_trigger;
  HomeAutomation::Components::R_TRIG room_2_trigger;
  HomeAutomation::Components::R_TRIG room_3_trigger;
  HomeAutomation::Components::R_TRIG room_4_trigger;
  HomeAutomation::Components::R_TRIG room_5_trigger;
  HomeAutomation::Components::R_TRIG room_6_trigger;
  HomeAutomation::Components::Light workshop_1_light{"Workshop 1"};
  HomeAutomation::Components::Light workshop_2_light{"Workshop 2"};
  HomeAutomation::Components::Light room_1_light{"Room 1"};
  HomeAutomation::Components::Light room_2_light{"Room 2"};
  HomeAutomation::Components::Light room_3_light{"Room 3"};
  HomeAutomation::Components::Light room_4_light{"Room 4"};
  HomeAutomation::Components::Light room_5_light{"Room 5"};
  HomeAutomation::Components::Light room_6_light{"Room 6"};
};

namespace HomeAutomation {
namespace Runtime {

std::shared_ptr<HomeAutomation::Scheduler::CppProgram>
createCppProgram(std::string const &name, HomeAutomation::GV *gv,
                 HomeAutomation::Components::MQTT::ClientPaho *mqtt) {
  if (name == "BasementLogic") {
    return std::make_shared<BasementLogic>(gv, mqtt);
  }
  spdlog::error("Unknown program named {} requested.", name);
  return std::shared_ptr<HomeAutomation::Scheduler::CppProgram>();
}

} // namespace Runtime
} // namespace HomeAutomation
