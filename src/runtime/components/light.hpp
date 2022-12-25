#pragma once

#include <spdlog/spdlog.h>

#include <string>

namespace HomeAutomation {
namespace Components {

class Light {
public:
  Light(std::string name) : name{name}, state(false) {
    spdlog::info("Light {}: {}", name, state ? "on" : "off");
  }
  Light() : Light("") {}

  bool toggle() {
    state = !state;
    spdlog::info("Light {}: {}", name, state ? "on" : "off");
    return state;
  }
  bool getState() const { return state; }

private:
  std::string name;
  bool state;
};
} // namespace Components
} // namespace HomeAutomation
