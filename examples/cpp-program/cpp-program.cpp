#include <blind.hpp>
#include <light.hpp>

#include <program.hpp>

#include <spdlog/spdlog.h>

#include <sstream>

// execution context (shall run in dedicated thread with given cycle time)
class CppProgram final : public HomeAutomation::Runtime::Program {

public:
  CppProgram() = default;

  void init(std::shared_ptr<HomeAutomation::GV> gv) override { (void)gv; }

  void execute(std::shared_ptr<HomeAutomation::GV> gv,
               HomeAutomation::TimeStamp now) override {
    (void)now;

    if (stairs_light_trigger.execute(std::get<bool>(gv->inputs["button_a"]))) {
      gv->outputs["light_a"] = stairs_light.toggle();
    }

    if (kitchen_light_trigger.execute(std::get<bool>(gv->inputs["button_b"]))) {
      gv->outputs["light_b"] = kitchen_light.toggle();
    }

    if (charger_trigger.execute(std::get<bool>(gv->inputs["button_c"]))) {
      gv->outputs["light_c"] = charger.toggle();
    }

    if (deck_trigger.execute(std::get<bool>(gv->inputs["button_d"]))) {
      gv->outputs["light_d"] = deck_light.toggle();
    }

    if (u_light_trigger.execute(std::get<bool>(gv->inputs["button_e"]))) {
      gv->outputs["light_e"] = u_light.toggle();
    }

    gv->outputs["light_remote"] =
        ground_office_trigger.execute(std::get<bool>(gv->inputs["button_f"]));
  }

private:
  // logic blocks
  HomeAutomation::Library::R_TRIG stairs_light_trigger;
  HomeAutomation::Library::R_TRIG kitchen_light_trigger;
  HomeAutomation::Library::R_TRIG charger_trigger;
  HomeAutomation::Library::R_TRIG deck_trigger;
  HomeAutomation::Library::R_TRIG ground_office_trigger;
  HomeAutomation::Library::R_TRIG u_light_trigger;
  HomeAutomation::Library::Light stairs_light{"A"};
  HomeAutomation::Library::Light kitchen_light{"B"};
  HomeAutomation::Library::Light charger{"C"};
  HomeAutomation::Library::Light deck_light{"D"};
  HomeAutomation::Library::Light u_light{"E"};
};

namespace HomeAutomation {
namespace Runtime {

std::shared_ptr<HomeAutomation::Runtime::Program>
createCppProgram(std::string const &name) {
  if (name == "CppProgram") {
    return std::make_shared<CppProgram>();
  }
  std::stringstream s;
  s << "unknown program named " << name << " requested";
  throw std::invalid_argument(s.str());
}

} // namespace Runtime
} // namespace HomeAutomation
