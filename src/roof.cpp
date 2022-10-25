#include <blind.hpp>
#include <i2c_bus.hpp>
#include <i2c_dev.hpp>
#include <scheduler.hpp>
#include <signal.hpp>

#include <CLI/CLI.hpp>

#include <chrono>
#include <iostream>
#include <memory>

static constexpr auto const cfg = HomeAutomation::Components::BlindConfig{
    .periodIdle = 500ms, .periodUp = 50s, .periodDown = 50s};

struct GV {
  struct {
    bool sr_raff_up;
    bool sr_raff_down;
    bool kizi_2_raff_up;
    bool kizi_2_raff_down;
  } inputs;

  struct {
    bool sr_raff_up;
    bool sr_raff_down;
    bool kizi_2_raff_up;
    bool kizi_2_raff_down;
  } outputs;
};

// execution context (shall run in dedicated thread with given cycle time)
class RoofLogic : public HomeAutomation::Logic::Program {

public:
  RoofLogic(GV &gv)
      : gv(gv), blind_sr(cfg, std::chrono::high_resolution_clock::now()),
        blind_kizi_2(cfg, std::chrono::high_resolution_clock::now()) {}

  void execute(HomeAutomation::TimeStamp now) override {
    auto result =
        blind_sr.execute(now, gv.inputs.sr_raff_up, gv.inputs.sr_raff_down);
    gv.outputs.sr_raff_up = result.up;
    gv.outputs.sr_raff_down = result.down;

    result = blind_kizi_2.execute(now, gv.inputs.kizi_2_raff_up,
                                  gv.inputs.kizi_2_raff_down);
    gv.outputs.kizi_2_raff_up = result.up;
    gv.outputs.kizi_2_raff_down = result.down;
  }

private:
  GV &gv;

  // logic blocks
  HomeAutomation::Components::Blind blind_sr;
  HomeAutomation::Components::Blind blind_kizi_2;
};

int main(int argc, char *argv[]) {
  using namespace std::chrono_literals;

  auto app = CLI::App("Roof PLC");
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

  auto pcf8574Input_3b = HomeAutomation::IO::I2C::PCF8574Input(0x3b);
  i2c_bus->RegisterInput(&pcf8574Input_3b);

  auto max7311Output = HomeAutomation::IO::I2C::MAX7311Output(0x20);
  i2c_bus->RegisterOutput(&max7311Output);

  // global variables
  GV gv{};

  // create tasks and programs
  auto &mainTask = scheduler.createTask(
      100ms, HomeAutomation::Logic::TaskCbs{
                 .init = [i2c_bus]() { i2c_bus->init(); },
                 .before =
                     [i2c_bus, &gv, &pcf8574Input_3b]() {
                       // perform real I/O
                       i2c_bus->readInputs();

                       // transfer into GV memory
                       gv.inputs.sr_raff_up = pcf8574Input_3b.getInput(0);
                       gv.inputs.sr_raff_down = pcf8574Input_3b.getInput(1);
                       gv.inputs.kizi_2_raff_up = pcf8574Input_3b.getInput(2);
                       gv.inputs.kizi_2_raff_down = pcf8574Input_3b.getInput(3);
                     },
                 .after =
                     [i2c_bus, &gv, &max7311Output]() {
                       // transfer from GV memory
                       max7311Output.setOutput(1, gv.outputs.sr_raff_up);
                       max7311Output.setOutput(0, gv.outputs.sr_raff_down);
                       max7311Output.setOutput(3, gv.outputs.kizi_2_raff_up);
                       max7311Output.setOutput(2, gv.outputs.kizi_2_raff_down);

                       // perform real I/O
                       i2c_bus->writeOutputs();
                     },
                 .shutdown = [i2c_bus]() { i2c_bus->close(); },
                 .quit = HomeAutomation::System::quitCondition});
  auto roofLogic = RoofLogic(gv);
  mainTask.addProgram(&roofLogic);

  // run
  scheduler.start();
  return scheduler.wait();
}
