#pragma once

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

private:
  bool last;
};

} // namespace Components
} // namespace HomeAutomation
