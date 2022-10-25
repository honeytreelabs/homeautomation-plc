#include <i2c_bus.hpp>
#include <i2c_dev.hpp>
#include <light.hpp>
#include <scheduler.hpp>
#include <signal.hpp>
#include <trigger.hpp>

#include <CLI/CLI.hpp>

#include <iostream>
#include <memory>

struct GV {
  struct {
    bool stairs_light;
    bool kitchen_light;
    bool charger;
    bool deck_light;
  } inputs;

  struct {
    bool stairs_light;
    bool kitchen_light;
    bool charger;
    bool deck_light;
  } outputs;
};

// execution context (shall run in dedicated thread with given cycle time)
class GroundLogic : public HomeAutomation::Logic::Program {

public:
  GroundLogic(GV &gv) : gv(gv), stairs_light_trigger{}, stairs_light{} {}

  void execute(HomeAutomation::TimeStamp now) override {
    (void)now;

    if (stairs_light_trigger.execute(gv.inputs.stairs_light)) {
      gv.outputs.stairs_light = stairs_light.toggle();
    }

    if (kitchen_light_trigger.execute(gv.inputs.kitchen_light)) {
      gv.outputs.kitchen_light = kitchen_light.toggle();
    }

    if (charger_trigger.execute(gv.inputs.charger)) {
      gv.outputs.charger = charger.toggle();
    }

    if (deck_trigger.execute(gv.inputs.deck_light)) {
      gv.outputs.deck_light = deck_light.toggle();
    }
  }

private:
  GV &gv;

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

  auto app = CLI::App("Ground PLC");
  std::string implementation = "real";
  app.add_option("-i,--implementation", implementation,
                 "Implementation variant (real, stub)");
  std::string i2c_bus_path = "/dev/i2c-1";
  app.add_option("-b,--bus", i2c_bus_path, "Path to the i2c bus device");
  CLI11_PARSE(app, argc, argv);

  HomeAutomation::System::initQuitCondition();

  auto scheduler = HomeAutomation::Logic::Scheduler();

  // create IO infrastructure
  std::shared_ptr<HomeAutomation::IO::I2C::Bus> i2c_bus;
  if (implementation == "stub") {
    i2c_bus = std::make_shared<HomeAutomation::IO::I2C::StubBus>("ignored");
  } else {
    i2c_bus = std::make_shared<HomeAutomation::IO::I2C::RealBus>(i2c_bus_path);
  }

  auto pcf8574Input_38 = HomeAutomation::IO::I2C::PCF8574Input(0x38);
  i2c_bus->RegisterInput(&pcf8574Input_38);

  auto pcf8574Output_20 = HomeAutomation::IO::I2C::PCF8574Output(0x20);
  i2c_bus->RegisterOutput(&pcf8574Output_20);

  // global variables
  GV gv{};

  // create tasks and programs
  auto &mainTask = scheduler.createTask(
      100ms, HomeAutomation::Logic::TaskCbs{
                 .init = [i2c_bus]() { i2c_bus->init(); },
                 .before =
                     [i2c_bus, &gv, &pcf8574Input_38]() {
                       // perform real I/O
                       i2c_bus->readInputs();

                       // transfer into GV memory
                       gv.inputs.stairs_light = pcf8574Input_38.getInput(1);
                       gv.inputs.kitchen_light = pcf8574Input_38.getInput(2);
                       gv.inputs.charger = pcf8574Input_38.getInput(3);
                       gv.inputs.deck_light = pcf8574Input_38.getInput(4);
                     },
                 .after =
                     [i2c_bus, &gv, &pcf8574Output_20]() {
                       // transfer from GV memory
                       pcf8574Output_20.setOutput(0, gv.outputs.charger);
                       pcf8574Output_20.setOutput(1, gv.outputs.stairs_light);
                       pcf8574Output_20.setOutput(2, gv.outputs.stairs_light);
                       pcf8574Output_20.setOutput(3, gv.outputs.kitchen_light);
                       pcf8574Output_20.setOutput(4, gv.outputs.deck_light);

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
}
