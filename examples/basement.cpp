// library
#include <light.hpp>
#include <trigger.hpp>

// PLC runtime
#include <gv.hpp>
#include <program.hpp>

#include <spdlog/spdlog.h>

#include <sstream>

// execution context (shall run in dedicated thread with given cycle time)
class BasementLogic final : public HomeAutomation::Runtime::CppProgram {

public:
  BasementLogic(HomeAutomation::GV *gv)
      : HomeAutomation::Runtime::CppProgram(gv) {}

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
  HomeAutomation::Library::R_TRIG workshop_1_trigger;
  HomeAutomation::Library::R_TRIG workshop_2_trigger;
  HomeAutomation::Library::R_TRIG room_1_trigger;
  HomeAutomation::Library::R_TRIG room_2_trigger;
  HomeAutomation::Library::R_TRIG room_3_trigger;
  HomeAutomation::Library::R_TRIG room_4_trigger;
  HomeAutomation::Library::R_TRIG room_5_trigger;
  HomeAutomation::Library::R_TRIG room_6_trigger;
  HomeAutomation::Library::Light workshop_1_light{"Workshop 1"};
  HomeAutomation::Library::Light workshop_2_light{"Workshop 2"};
  HomeAutomation::Library::Light room_1_light{"Room 1"};
  HomeAutomation::Library::Light room_2_light{"Room 2"};
  HomeAutomation::Library::Light room_3_light{"Room 3"};
  HomeAutomation::Library::Light room_4_light{"Room 4"};
  HomeAutomation::Library::Light room_5_light{"Room 5"};
  HomeAutomation::Library::Light room_6_light{"Room 6"};
};

namespace HomeAutomation {
namespace Runtime {

std::shared_ptr<HomeAutomation::Runtime::CppProgram>
createCppProgram(std::string const &name, HomeAutomation::GV *gv) {
  if (name == "BasementLogic") {
    return std::make_shared<BasementLogic>(gv);
  }
  std::stringstream s;
  s << "unknown program named " << name << " requested";
  throw std::invalid_argument(s.str());
}

} // namespace Runtime
} // namespace HomeAutomation
