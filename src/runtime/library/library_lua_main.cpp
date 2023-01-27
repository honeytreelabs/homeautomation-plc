#include <library_registry_lua.hpp>

#include <sol/sol.hpp>
#include <sol/state_handling.hpp>
#include <spdlog/spdlog.h>

#include <cstdlib>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    spdlog::error("Wrong number of arguments. Usage: {} <path-to-script>");
    return EXIT_FAILURE;
  }

  sol::state lua;
  lua.open_libraries(/* all standard libraries */);

  HomeAutomation::Library::LuaLibraryRegistry::RegisterComponents(lua);

  try {
    return lua.script_file(argv[1]).valid() ? EXIT_SUCCESS : EXIT_FAILURE;
  } catch (sol::error const &exc) {
    spdlog::error("Error executing the provided script: {}", exc.what());
    return EXIT_FAILURE;
  }
}
