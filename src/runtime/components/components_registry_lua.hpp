#pragma once

#include <sol/sol.hpp>

namespace HomeAutomation {
namespace Components {

class LuaComponentsRegistry {
public:
  static void RegisterComponents(sol::state &lua);
};

} // namespace Components
} // namespace HomeAutomation
