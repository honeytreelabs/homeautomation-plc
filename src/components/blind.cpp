#include <blind.hpp>

namespace HomeAutomation {
namespace Components {

OptionalBlindState BlindStateIdle::execute(TimeStamp now, bool up, bool down) {
  spdlog::debug("BlindStateIdle::execute");

  auto up_triggered = up_trigger.execute(up);
  auto down_triggered = down_trigger.execute(down);

  if (now - start < cfg.periodIdle) {
    return NoTransition;
  }

  if (up_triggered) {
    return std::make_unique<BlindStateUp>(cfg, now, up, down);
  } else if (down_triggered) {
    return std::make_unique<BlindStateDown>(cfg, now, up, down);
  }
  return NoTransition;
}

Blind::Blind(BlindConfig const &cfg, TimeStamp const &now)
    : fsm(std::make_unique<BlindStateIdle>(cfg, now)) {}

BlindOutputs Blind::execute(TimeStamp now, bool button_up, bool button_down) {
  fsm.execute(now, button_up, button_down);
  return fsm.getStateData();
}

} // namespace Components
} // namespace HomeAutomation
