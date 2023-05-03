#include <blind_state.hpp>

namespace HomeAutomation {
namespace Library {

using namespace std::chrono_literals;

BlindConfig BlindConfigFromMillis(std::uint32_t periodIdle,
                                  std::uint32_t periodUp,
                                  std::uint32_t periodDown) {
  return BlindConfig{periodIdle * 1ms, periodUp * 1ms, periodDown * 1ms};
}

OptionalStateVariant transition(BlindStateIdle &idle, BlindContext &context) {
  auto const up_triggered = idle.up_trigger.execute(context.inputs.up);
  auto const down_triggered = idle.down_trigger.execute(context.inputs.down);

  if (context.now - idle.start < context.cfg.periodIdle) {
    return std::nullopt;
  }
  if (up_triggered) {
    return BlindStateUp(context.now, context.inputs.up, context.inputs.down);
  }
  if (down_triggered) {
    return BlindStateDown(context.now, context.inputs.up, context.inputs.down);
  }
  return std::nullopt;
}

OptionalStateVariant transition(BlindStateUp &up, BlindContext &context) {
  if (context.now - up.start > context.cfg.periodUp ||
      up.up_trigger.execute(context.inputs.up) ||
      up.down_trigger.execute(context.inputs.down)) {
    return BlindStateIdle(context.now, context.inputs.up, context.inputs.down);
  }
  return std::nullopt;
}

OptionalStateVariant transition(BlindStateDown &down, BlindContext &context) {
  if (context.now - down.start > context.cfg.periodDown ||
      down.up_trigger.execute(context.inputs.up) ||
      down.down_trigger.execute(context.inputs.down)) {
    return BlindStateIdle(context.now, context.inputs.up, context.inputs.down);
  }
  return std::nullopt;
}

} // namespace Library
} // namespace HomeAutomation
