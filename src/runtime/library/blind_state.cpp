#include <blind_state.hpp>

namespace HomeAutomation {
namespace Library {

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

} // namespace Library
} // namespace HomeAutomation
