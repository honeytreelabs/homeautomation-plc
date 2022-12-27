#pragma once

#include <sol/sol.hpp>

namespace HomeAutomation {
namespace Library {

class LuaLibraryRegistry {
public:
  static void RegisterComponents(sol::state &lua);
};

} // namespace Library
} // namespace HomeAutomation
