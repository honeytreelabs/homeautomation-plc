#pragma once

#include <sol/sol.hpp>

namespace HomeAutomation {
namespace Components {

class R_TRIG {
public:
  R_TRIG() : R_TRIG{false} {}
  R_TRIG(bool last) : last{last} {}

  bool execute(bool cur) {
    bool ret = !last && cur;
    last = cur;
    return ret;
  }

  static void RegisterComponent(sol::state &lua) {
    sol::usertype<R_TRIG> trigger_type = lua.new_usertype<R_TRIG>(
        "R_TRIG", sol::constructors<R_TRIG(), R_TRIG(bool)>());
    trigger_type["execute"] = &R_TRIG::execute;
  }

private:
  bool last;
};

class F_TRIG {
public:
  F_TRIG() : F_TRIG{false} {}
  F_TRIG(bool last) : last{last} {}

  bool execute(bool cur) {
    bool ret = last && !cur;
    last = cur;
    return ret;
  }

  static void RegisterComponent(sol::state &lua) {
    sol::usertype<F_TRIG> trigger_type = lua.new_usertype<F_TRIG>(
        "F_TRIG", sol::constructors<F_TRIG(), F_TRIG(bool)>());
    trigger_type["execute"] = &F_TRIG::execute;
  }

private:
  bool last;
};

} // namespace Components
} // namespace HomeAutomation
