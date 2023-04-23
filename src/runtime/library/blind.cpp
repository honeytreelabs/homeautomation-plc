#include <blind.hpp>

namespace HomeAutomation {
namespace Library {

using namespace std::chrono_literals;

Blind::Blind(BlindConfig const &cfg, TimeStamp const &now)
    : fsm{BlindContext{cfg, now}} {
  spdlog::info("Blind(idle = {} ms, up = {} ms, down = {} ms)",
               cfg.periodIdle / 1ms, cfg.periodUp / 1ms, cfg.periodDown / 1ms);
}

BlindOutputs Blind::execute(TimeStamp now, bool button_up, bool button_down) {
  fsm.context().now = now;
  fsm.context().inputs = {button_up, button_down};
  fsm.update();
  return BlindOutputs{fsm.context().outputs.up, fsm.context().outputs.down};
}

} // namespace Library
} // namespace HomeAutomation
