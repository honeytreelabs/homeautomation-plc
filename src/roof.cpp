// components
#include <blind.hpp>
#include <light.hpp>

// PLC runtime
#include <entry.hpp>
#include <gv.hpp>
#include <mqtt.hpp>
#include <scheduler.hpp>

#include <spdlog/spdlog.h>

static constexpr auto const cfg = HomeAutomation::Components::BlindConfig{
    .periodIdle = 500ms, .periodUp = 50s, .periodDown = 50s};

// execution context (shall run in dedicated thread with given cycle time)
class RoofLogic final : public HomeAutomation::Scheduler::CppProgram {

public:
  RoofLogic(HomeAutomation::GV *gv,
            HomeAutomation::Components::MQTT::ClientPaho *mqtt)
      : HomeAutomation::Scheduler::CppProgram(gv, mqtt), blind_sr(cfg),
        blind_kizi_2(cfg) {}

  void execute(HomeAutomation::TimeStamp now) override {
    // blind: sr_raff
    auto result =
        blind_sr.execute(now, std::get<bool>(gv->inputs["sr_raff_up"]),
                         std::get<bool>(gv->inputs["sr_raff_down"]));
    gv->outputs["sr_raff_up"] = result.up;
    gv->outputs["sr_raff_down"] = result.down;

    // blind: kizi_2_raff
    result =
        blind_kizi_2.execute(now, std::get<bool>(gv->inputs["kizi_2_raff_up"]),
                             std::get<bool>(gv->inputs["kizi_2_raff_down"]));
    gv->outputs["kizi_2_raff_up"] = result.up;
    gv->outputs["kizi_2_raff_down"] = result.down;

    // light: ground_office
    auto msg = mqtt->receive();
    if (msg && msg->get_topic() == "/homeautomation/ground_office_light" &&
        msg->get_payload_str() == "toggle") {
      gv->outputs["ground_office_light"] = ground_office_light.toggle();
    }
  }

private:
  // logic blocks
  HomeAutomation::Components::Blind blind_sr;
  HomeAutomation::Components::Blind blind_kizi_2;
  HomeAutomation::Components::Light ground_office_light{"Ground Office"};
};

namespace HomeAutomation {
namespace Runtime {

std::shared_ptr<HomeAutomation::Scheduler::CppProgram>
createCppProgram(std::string const &name, HomeAutomation::GV *gv,
                 HomeAutomation::Components::MQTT::ClientPaho *mqtt) {
  if (name == "RoofLogic") {
    return std::make_shared<RoofLogic>(gv, mqtt);
  }
  spdlog::error("Unknown program named {} requested.", name);
  return std::shared_ptr<HomeAutomation::Scheduler::CppProgram>();
}

} // namespace Runtime
} // namespace HomeAutomation
