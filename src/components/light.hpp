#pragma once

namespace HomeAutomation {
namespace Components {

class Light {
public:
  Light() : state(false) {}

  bool toggle() {
    state = !state;
    return state;
  }
  bool getState() const { return state; }

private:
  bool state;
};
} // namespace Components
} // namespace HomeAutomation
