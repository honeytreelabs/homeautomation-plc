#pragma once

#include <sol/sol.hpp>

namespace HomeAutomation {
namespace Library {

class LuaLibraryRegistry {
public:
  LuaLibraryRegistry() = delete;
  ~LuaLibraryRegistry() = delete;
  static void RegisterComponents(sol::state &lua);
};

} // namespace Library
} // namespace HomeAutomation
