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

  static void RegisterComponent(sol::state &lua) {
    sol::usertype<BlindOutputs> blind_outputs_type =
        lua.new_usertype<BlindOutputs>("BlindOutputs");
    blind_outputs_type["up"] = &BlindOutputs::up;
    blind_outputs_type["down"] = &BlindOutputs::down;

    lua["BlindConfigFromMillis"] = &BlindConfigFromMillis;

    sol::usertype<Blind> blind_type = lua.new_usertype<Blind>(
        "Blind", sol::constructors<Blind(BlindConfig const &)>());
    blind_type["execute"] = &Blind::execute;
  }

private:
  BlindFSM fsm;
};

} // namespace Library
} // namespace HomeAutomation
