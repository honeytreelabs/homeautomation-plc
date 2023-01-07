#include <library_registry_lua.hpp>
#include <program_lua.hpp>

#include <spdlog/spdlog.h>
#include <stdexcept>

namespace HomeAutomation {
namespace Runtime {

LuaProgram::LuaProgram(std::filesystem::path const &path) : lua{} {
  lua.open_libraries(/* all standard libraries */);

  // forward GVs into program
  sol::usertype<HomeAutomation::GV> gv_type =
      lua.new_usertype<HomeAutomation::GV>("GVType");
  gv_type["inputs"] = &HomeAutomation::GV::inputs;
  gv_type["outputs"] = &HomeAutomation::GV::outputs;

  HomeAutomation::Library::LuaLibraryRegistry::RegisterComponents(lua);

  try {
    lua.script_file(path);
  } catch (sol::error const &err) {
    throw std::invalid_argument(err.what());
  }
}

void LuaProgram::init(std::shared_ptr<HomeAutomation::GV> gv) {
  lua["Init"](gv.get());
}

void LuaProgram::execute(std::shared_ptr<HomeAutomation::GV> gv,
                         TimeStamp now) {
  lua["Cycle"](gv.get(), now);
}

} // namespace Runtime
} // namespace HomeAutomation
