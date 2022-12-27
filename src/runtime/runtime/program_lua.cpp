#include <library_registry_lua.hpp>
#include <program_lua.hpp>

#include <spdlog/spdlog.h>

namespace HomeAutomation {
namespace Runtime {

LuaProgram::LuaProgram(std::filesystem::path const &path,
                       HomeAutomation::GV *gv)
    : path{path}, gv{gv}, lua{} {
  lua.open_libraries(/* all standard libraries */);

  // forward GVs into program
  sol::usertype<HomeAutomation::GV> gv_type =
      lua.new_usertype<HomeAutomation::GV>("GVType");
  gv_type["inputs"] = &HomeAutomation::GV::inputs;
  gv_type["outputs"] = &HomeAutomation::GV::outputs;
  lua["GV"] = gv;

  HomeAutomation::Library::LuaLibraryRegistry::RegisterComponents(lua);
}

void LuaProgram::execute(TimeStamp now) {
  lua["NOW"] = now;

  lua.script_file(path);
}

} // namespace Runtime
} // namespace HomeAutomation
