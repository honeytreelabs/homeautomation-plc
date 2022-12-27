// components
#include <blind.hpp>
#include <light.hpp>

// PLC runtime
#include <gv.hpp>
#include <program.hpp>

#include <spdlog/spdlog.h>

#include <sstream>

using namespace std::chrono_literals;

static constexpr auto const cfg = HomeAutomation::Components::BlindConfig{
    .periodIdle = 500ms, .periodUp = 50s, .periodDown = 50s};

// execution context (shall run in dedicated thread with given cycle time)
class RoofLogic final : public HomeAutomation::Runtime::CppProgram {

public:
  RoofLogic(HomeAutomation::GV *gv)
      : HomeAutomation::Runtime::CppProgram(gv), blind_sr(cfg),
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
    if (std::get<bool>(gv->inputs["ground_office_light"])) {
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

std::shared_ptr<HomeAutomation::Runtime::CppProgram>
createCppProgram(std::string const &name, HomeAutomation::GV *gv) {
  if (name == "RoofLogic") {
    return std::make_shared<RoofLogic>(gv);
  }
  std::stringstream s;
  s << "unknown program named " << name << " requested";
  throw std::invalid_argument(s.str());
}

} // namespace Runtime
} // namespace HomeAutomation
