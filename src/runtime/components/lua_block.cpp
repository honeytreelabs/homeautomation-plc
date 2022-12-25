#include <lua_block.hpp>

namespace HomeAutomation {
namespace Components {

Lua::Lua(std::string path) : path{path}, lua{} {
  lua.open_libraries();
  lua.new_usertype<GVWrapper>("GV", "getInput", &GVWrapper::getInput,
                              "getOutput", &GVWrapper::getOutput);
  lua.new_usertype<VarValueWrapper>(
      "VarValue", "type_id", &VarValueWrapper::type_id, "as_bool",
      &VarValueWrapper::as_bool, "as_int", &VarValueWrapper::as_int, "set_bool",
      &VarValueWrapper::set_bool, "set_int", &VarValueWrapper::set_int);
}

void Lua::execute(GV &gv) {
  GVWrapper gvWrapper(gv);
  lua["gv"] = &gvWrapper;
  lua.script_file(path);
}

} // namespace Components
} // namespace HomeAutomation
