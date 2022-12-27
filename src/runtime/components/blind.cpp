#include <blind.hpp>

namespace HomeAutomation {
namespace Components {

Blind::Blind(BlindConfig const &cfg, TimeStamp const &now)
    : fsm(std::make_unique<BlindStateIdle>(cfg, now)) {}

BlindOutputs Blind::execute(TimeStamp now, bool button_up, bool button_down) {
  fsm.execute(now, button_up, button_down);
  return fsm.getStateData();
}

} // namespace Components
} // namespace HomeAutomation
