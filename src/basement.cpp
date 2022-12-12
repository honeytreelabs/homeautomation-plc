#include <config.hpp>
#include <gv.hpp>
#include <i2c_bus.hpp>
#include <i2c_dev.hpp>
#include <light.hpp>
#include <modbus.hpp>
#include <scheduler.hpp>
#include <signal.hpp>
#include <trigger.hpp>

#include <iostream>
#include <memory>

// execution context (shall run in dedicated thread with given cycle time)
class GroundLogic : public HomeAutomation::Scheduler::Program {

public:
  GroundLogic(HomeAutomation::GV &gv)
      : gv(gv), stairs_light_trigger{}, stairs_light{} {}

  void execute(HomeAutomation::TimeStamp now) override {
    (void)now;

    if (stairs_light_trigger.execute(
            std::get<bool>(gv.inputs["stairs_light"]))) {
      gv.outputs["stairs_light"] = stairs_light.toggle();
    }

    if (kitchen_light_trigger.execute(
            std::get<bool>(gv.inputs["kitchen_light"]))) {
      gv.outputs["kitchen_light"] = kitchen_light.toggle();
    }

    if (charger_trigger.execute(std::get<bool>(gv.inputs["charger"]))) {
      gv.outputs["charger"] = charger.toggle();
    }

    if (deck_trigger.execute(std::get<bool>(gv.inputs["deck_light"]))) {
      gv.outputs["deck_light"] = deck_light.toggle();
    }
  }

private:
  HomeAutomation::GV &gv;

  // logic blocks
  HomeAutomation::Components::R_TRIG stairs_light_trigger;
  HomeAutomation::Components::R_TRIG kitchen_light_trigger;
  HomeAutomation::Components::R_TRIG charger_trigger;
  HomeAutomation::Components::R_TRIG deck_trigger;
  HomeAutomation::Components::Light stairs_light;
  HomeAutomation::Components::Light kitchen_light;
  HomeAutomation::Components::Light charger;
  HomeAutomation::Components::Light deck_light;
};

int main(int argc, char *argv[]) {
  using namespace std::chrono_literals;

  if (argc != 2) {
    spdlog::error("Usage: {} <path-to-config-file>", argv[0]);
    return 1;
  }

  std::string const implementation = "real"; // alternative: stub

  try {
    auto config = HomeAutomation::Config::fromFile(argv[1]);

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

    auto pcf8574Input_38 = HomeAutomation::IO::I2C::PCF8574Input(0x38);
    i2c_bus->RegisterInput(&pcf8574Input_38);

    auto pcf8574Output_20 = HomeAutomation::IO::I2C::PCF8574Output(0x20);
    i2c_bus->RegisterOutput(&pcf8574Output_20);

    // global variables
    HomeAutomation::GV gv{{{"stairs_light", false}, // inputs
                           {"kitchen_light", false},
                           {"charger", false},
                           {"deck_light", false}},
                          {{"stairs_light", false}, // outputs
                           {"kitchen_light", false},
                           {"charger", false},
                           {"deck_light", false}}};

    // create tasks and programs
    auto &mainTask = scheduler.createTask(
        100ms,
        HomeAutomation::Scheduler::TaskCbs{
            .init = [i2c_bus]() { i2c_bus->init(); },
            .before =
                [i2c_bus, &gv, &pcf8574Input_38]() {
                  // perform real I/O
                  i2c_bus->readInputs();

                  // transfer into GV memory
                  gv.inputs["stairs_light"] = pcf8574Input_38.getInput(1);
                  gv.inputs["kitchen_light"] = pcf8574Input_38.getInput(2);
                  gv.inputs["charger"] = pcf8574Input_38.getInput(3);
                  gv.inputs["deck_light"] = pcf8574Input_38.getInput(4);
                },
            .after =
                [i2c_bus, &gv, &pcf8574Output_20]() {
                  // transfer from GV memory
                  pcf8574Output_20.setOutput(
                      0, std::get<bool>(gv.outputs["charger"]));
                  pcf8574Output_20.setOutput(
                      1, std::get<bool>(gv.outputs["stairs_light"]));
                  pcf8574Output_20.setOutput(
                      2, std::get<bool>(gv.outputs["stairs_light"]));
                  pcf8574Output_20.setOutput(
                      3, std::get<bool>(gv.outputs["kitchen_light"]));
                  pcf8574Output_20.setOutput(
                      4, std::get<bool>(gv.outputs["deck_light"]));

                  // perform real I/O
                  i2c_bus->writeOutputs();
                },
            .shutdown = [i2c_bus]() { i2c_bus->close(); },
            .quit = HomeAutomation::System::quitCondition});
    auto groundLogic = GroundLogic(gv);
    mainTask.addProgram(&groundLogic);

    // run
    scheduler.start();
    return scheduler.wait();
  } catch (YAML::Exception const &exc) {
    spdlog::error("Could not parse configuration file: {}", exc.what());
    return 1;
  }
}
