#pragma once

#include <blind_state.hpp>

namespace HomeAutomation {
namespace Library {

class Blind {
public:
  Blind(BlindConfig const &cfg)
      : Blind(cfg, std::chrono::high_resolution_clock::now()) {}
  Blind(BlindConfig const &cfg, TimeStamp const &now);
  BlindOutputs execute(TimeStamp now, bool button_up, bool button_down);

private:
  BlindFSM fsm;
};

} // namespace Library
} // namespace HomeAutomation
