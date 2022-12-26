#include <program_lua.hpp>

#include <spdlog/spdlog.h>

namespace HomeAutomation {
namespace Runtime {

LuaProgram::LuaProgram(std::filesystem::path const &path,
                       HomeAutomation::GV *gv)
    : path{path}, gv{gv}, lua{} {
  lua.open_libraries();
  // TODO make components available in Lua interpreter
  sol::usertype<HomeAutomation::GV> gv_type =
      lua.new_usertype<HomeAutomation::GV>("GVType");
  gv_type["inputs"] = &HomeAutomation::GV::inputs;
  gv_type["outputs"] = &HomeAutomation::GV::outputs;
  lua["GV"] = gv;
}

void LuaProgram::execute(TimeStamp now) {
  // transfer runtime variables
  lua["NOW"] = now.time_since_epoch();

  lua.script_file(path);
}

} // namespace Runtime
} // namespace HomeAutomation
