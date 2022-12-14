#include <blind_state.hpp>

namespace HomeAutomation {
namespace Library {

BlindConfig BlindConfigFromMillis(std::uint32_t periodIdle,
                                  std::uint32_t periodUp,
                                  std::uint32_t periodDown) {
  return BlindConfig{periodIdle * 1ms, periodUp * 1ms, periodDown * 1ms};
}

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
