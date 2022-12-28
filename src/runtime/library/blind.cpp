#include <blind.hpp>

namespace HomeAutomation {
namespace Library {

Blind::Blind(BlindConfig const &cfg, TimeStamp const &now)
    : fsm(std::make_unique<BlindStateIdle>(cfg, now)) {
  spdlog::info("Blind(idle = {} ms, up = {} ms, down = {} ms)",
               cfg.periodIdle / 1ms, cfg.periodUp / 1ms, cfg.periodDown / 1ms);
}

BlindOutputs Blind::execute(TimeStamp now, bool button_up, bool button_down) {
  fsm.execute(now, button_up, button_down);
  return fsm.getStateData();
}

} // namespace Library
} // namespace HomeAutomation
