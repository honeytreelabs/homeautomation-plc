#include <blind.hpp>

namespace HomeAutomation {
namespace Library {

Blind::Blind(BlindConfig const &cfg, TimeStamp const &now)
    : fsm(std::make_unique<BlindStateIdle>(cfg, now)) {}

BlindOutputs Blind::execute(TimeStamp now, bool button_up, bool button_down) {
  fsm.execute(now, button_up, button_down);
  return fsm.getStateData();
}

} // namespace Library
} // namespace HomeAutomation
