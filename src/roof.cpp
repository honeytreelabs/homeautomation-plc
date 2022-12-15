#include <blind.hpp>
#include <config.hpp>
#include <gv.hpp>
#include <i2c_bus.hpp>
#include <i2c_dev.hpp>
#include <light.hpp>
#include <mqtt.hpp>
#include <runtime_factory.hpp>
#include <scheduler.hpp>
#include <signal.hpp>

#include <spdlog/spdlog.h>

#include <chrono>
#include <iostream>
#include <memory>

static constexpr auto const cfg = HomeAutomation::Components::BlindConfig{
    .periodIdle = 500ms, .periodUp = 50s, .periodDown = 50s};

// execution context (shall run in dedicated thread with given cycle time)
class RoofLogic : public HomeAutomation::Scheduler::Program {

public:
  RoofLogic(HomeAutomation::GV &gv,
            HomeAutomation::Components::MQTT::ClientPaho &mqtt)
      : gv(gv), mqtt{mqtt}, blind_sr(cfg), blind_kizi_2(cfg) {}
  virtual ~RoofLogic() {}

  void execute(HomeAutomation::TimeStamp now) override {
    // blind: sr_raff
    auto result = blind_sr.execute(now, std::get<bool>(gv.inputs["sr_raff_up"]),
                                   std::get<bool>(gv.inputs["sr_raff_down"]));
    gv.outputs["sr_raff_up"] = result.up;
    gv.outputs["sr_raff_down"] = result.down;

    // blind: kizi_2_raff
    result =
        blind_kizi_2.execute(now, std::get<bool>(gv.inputs["kizi_2_raff_up"]),
                             std::get<bool>(gv.inputs["kizi_2_raff_down"]));
    gv.outputs["kizi_2_raff_up"] = result.up;
    gv.outputs["kizi_2_raff_down"] = result.down;

    // light: ground_office
    auto msg = mqtt.receive();
    if (msg && msg->get_topic() == "/homeautomation/ground_office_light" &&
        msg->get_payload_str() == "toggle") {
      gv.outputs["ground_office_light"] = ground_office_light.toggle();
    }
  }

private:
  HomeAutomation::GV &gv;
  HomeAutomation::Components::MQTT::ClientPaho &mqtt;

  // logic blocks
  HomeAutomation::Components::Blind blind_sr;
  HomeAutomation::Components::Blind blind_kizi_2;
  HomeAutomation::Components::Light ground_office_light{"Ground Office"};
};

int main(int argc, char *argv[]) {
  using namespace std::chrono_literals;

  if (argc != 2) {
    spdlog::error("Usage: {} <path-to-config-file>", argv[0]);
    return 1;
  }

  try {
    HomeAutomation::System::initQuitCondition();

    auto runtime = HomeAutomation::Runtime::RuntimeFactory::fromFile(argv[1]);

    auto roofLogic = RoofLogic(*runtime->GV(),
                               *runtime->Scheduler()->getTask("main")->MQTT());
    runtime->Scheduler()->getTask("main")->addProgram(&roofLogic);

    runtime->start(HomeAutomation::System::quitCondition);
    return runtime->wait();
  } catch (YAML::Exception const &exc) {
    spdlog::error("Could not parse configuration file: {}", exc.what());
    return 1;
  } catch (mqtt::exception &exc) {
    spdlog::error(
        "Some error occurred when interacting with the MQTT broker: {}",
        exc.what());
    return 1;
  }
}
