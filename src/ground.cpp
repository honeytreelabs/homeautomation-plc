// components
#include <blind.hpp>
#include <light.hpp>

// PLC runtime
#include <entry.hpp>
#include <gv.hpp>
#include <mqtt.hpp>
#include <scheduler.hpp>

#include <spdlog/spdlog.h>

// execution context (shall run in dedicated thread with given cycle time)
class GroundLogic final : public HomeAutomation::Scheduler::CppProgram {

public:
  GroundLogic(HomeAutomation::GV *gv,
              HomeAutomation::Components::MQTT::ClientPaho *mqtt)
      : CppProgram(gv, mqtt), stairs_light_trigger{}, kitchen_light_trigger{},
        charger_trigger{}, deck_trigger{}, ground_office_trigger{},
        stairs_light{"Stairs"}, kitchen_light{"Kitchen"}, charger{"Charger"},
        deck_light{"Deck"} {}

  void execute(HomeAutomation::TimeStamp now) override {
    (void)now;

    if (stairs_light_trigger.execute(
            std::get<bool>(gv->inputs["stairs_light"]))) {
      gv->outputs["stairs_light"] = stairs_light.toggle();
    }

    if (kitchen_light_trigger.execute(
            std::get<bool>(gv->inputs["kitchen_light"]))) {
      gv->outputs["kitchen_light"] = kitchen_light.toggle();
    }

    if (charger_trigger.execute(std::get<bool>(gv->inputs["charger"]))) {
      gv->outputs["charger"] = charger.toggle();
    }

    if (deck_trigger.execute(std::get<bool>(gv->inputs["deck_light"]))) {
      gv->outputs["deck_light"] = deck_light.toggle();
    }

    if (ground_office_trigger.execute(
            std::get<bool>(gv->inputs["ground_office_light"]))) {
      mqtt->send("/homeautomation/ground_office_light", "toggle", 1);
    }
  }

private:
  // logic blocks
  HomeAutomation::Components::R_TRIG stairs_light_trigger;
  HomeAutomation::Components::R_TRIG kitchen_light_trigger;
  HomeAutomation::Components::R_TRIG charger_trigger;
  HomeAutomation::Components::R_TRIG deck_trigger;
  HomeAutomation::Components::R_TRIG ground_office_trigger;
  HomeAutomation::Components::Light stairs_light;
  HomeAutomation::Components::Light kitchen_light;
  HomeAutomation::Components::Light charger;
  HomeAutomation::Components::Light deck_light;
};

namespace HomeAutomation {
namespace Runtime {

std::shared_ptr<HomeAutomation::Scheduler::CppProgram>
createCppProgram(std::string const &name, HomeAutomation::GV *gv,
                 HomeAutomation::Components::MQTT::ClientPaho *mqtt) {
  if (name == "GroundLogic") {
    return std::make_shared<GroundLogic>(gv, mqtt);
  }
  spdlog::error("Unknown program named {} requested.", name);
  return std::shared_ptr<HomeAutomation::Scheduler::CppProgram>();
}

} // namespace Runtime
} // namespace HomeAutomation
