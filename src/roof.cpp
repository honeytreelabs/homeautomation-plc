#include <blind.hpp>
#include <config.hpp>
#include <gv.hpp>
#include <i2c_bus.hpp>
#include <i2c_dev.hpp>
#include <light.hpp>
#include <mqtt.hpp>
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
  RoofLogic(HomeAutomation::Config const &config, HomeAutomation::GV &gv)
      : gv(gv), mqtt{config.mqtt.address, config.mqtt.clientID,
                     getConnectOptions(config.mqtt.username,
                                       config.mqtt.password)},
        blind_sr(cfg, std::chrono::high_resolution_clock::now()),
        blind_kizi_2(cfg, std::chrono::high_resolution_clock::now()) {
    mqtt.connect();
    mqtt.subscribe("/homeautomation/ground_office_light");
  }
  virtual ~RoofLogic() { mqtt.disconnect(); }

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
    if (msg && msg->get_payload_str() == "toggle") {
      gv.outputs["ground_office_light"] = ground_office_light.toggle();
    }
  }

private:
  static mqtt::connect_options getConnectOptions(std::string username,
                                                 std::string password) {
    auto result =
        HomeAutomation::Components::MQTT::Client::getDefaultConnectOptions();
    result.set_user_name(username);
    result.set_password(password);
    return result;
  }

  HomeAutomation::GV &gv;
  HomeAutomation::Components::MQTT::Client mqtt;

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

  std::string const implementation = "real"; // alternative: stub
  std::string const config_path(argv[1]);

  try {
    auto config = HomeAutomation::Config::fromFile(config_path);

    HomeAutomation::System::initQuitCondition();

    auto scheduler = HomeAutomation::Scheduler::Scheduler();

    // create IO infrastructure
    std::shared_ptr<HomeAutomation::IO::I2C::Bus> i2c_bus;
    if (implementation == "stub") {
      i2c_bus = std::make_shared<HomeAutomation::IO::I2C::StubBus>("ignored");
    } else {
      i2c_bus =
          std::make_shared<HomeAutomation::IO::I2C::RealBus>(config.i2c.bus);
    }

    auto pcf8574Input_3b = HomeAutomation::IO::I2C::PCF8574Input(0x3b);
    i2c_bus->RegisterInput(&pcf8574Input_3b);

    auto max7311Output = HomeAutomation::IO::I2C::MAX7311Output(0x20);
    i2c_bus->RegisterOutput(&max7311Output);

    // global variables
    HomeAutomation::GV gv{{{"sr_raff_up", false}, // inputs
                           {"sr_raff_down", false},
                           {"kizi_2_raff_up", false},
                           {"kizi_2_raff_down", false}},
                          {{"sr_raff_up", false}, // outputs
                           {"sr_raff_down", false},
                           {"kizi_2_raff_up", false},
                           {"kizi_2_raff_down", false},
                           {"ground_office_light", false}}};

    // create tasks and programs
    auto &mainTask = scheduler.createTask(
        100ms,
        HomeAutomation::Scheduler::TaskCbs{
            .init = [i2c_bus]() { i2c_bus->init(); },
            .before =
                [i2c_bus, &gv, &pcf8574Input_3b]() {
                  // perform real I/O
                  i2c_bus->readInputs();

                  // transfer into GV memory
                  gv.inputs["sr_raff_up"] = pcf8574Input_3b.getInput(0);
                  gv.inputs["sr_raff_down"] = pcf8574Input_3b.getInput(1);
                  gv.inputs["kizi_2_raff_up"] = pcf8574Input_3b.getInput(2);
                  gv.inputs["kizi_2_raff_down"] = pcf8574Input_3b.getInput(3);
                },
            .after =
                [i2c_bus, &gv, &max7311Output]() {
                  // transfer from GV memory
                  max7311Output.setOutput(
                      1, std::get<bool>(gv.outputs["sr_raff_up"]));
                  max7311Output.setOutput(
                      0, std::get<bool>(gv.outputs["sr_raff_down"]));
                  max7311Output.setOutput(
                      3, std::get<bool>(gv.outputs["kizi_2_raff_up"]));
                  max7311Output.setOutput(
                      2, std::get<bool>(gv.outputs["kizi_2_raff_down"]));
                  max7311Output.setOutput(
                      4, std::get<bool>(gv.outputs["ground_office_light"]));

                  // perform real I/O
                  i2c_bus->writeOutputs();
                },
            .shutdown = [i2c_bus]() { i2c_bus->close(); },
            .quit = HomeAutomation::System::quitCondition});

    auto roofLogic = RoofLogic(config, gv);
    mainTask.addProgram(&roofLogic);
    scheduler.start();
    return scheduler.wait();
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
