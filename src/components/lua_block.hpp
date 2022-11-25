#pragma once

#include <gv.hpp>

#include <sol/sol.hpp>

#include <cstdint>
#include <map>
#include <variant>

namespace HomeAutomation {

namespace Components {

class VarValueWrapper {
public:
  VarValueWrapper(VarValue &val) : value(val) {}
  std::size_t type_id() { return value.index(); }
  bool as_bool() { return std::get<bool>(value); }
  int as_int() { return std::get<int>(value); }
  void set_bool(bool v) { // TODO check with std::holds_alternative
    value = v;
  }
  void set_int(int v) { // TODO check with std::holds_alternative
    value = v;
  }

private:
  VarValue &value;
};

class GVWrapper {
public:
  GVWrapper(GV &gv) : gv(gv) {}

  VarValueWrapper getInput(std::string key) {
    return VarValueWrapper(gv.inputs[key]);
  }
  VarValueWrapper getOutput(std::string key) {
    return VarValueWrapper(gv.outputs[key]);
  }

private:
  GV &gv;
};

class Lua {
public:
  Lua(std::string path);
  virtual ~Lua() {}
  virtual void execute(GV &gv);

private:
  Lua();
  Lua(Lua const &);
  Lua &operator=(Lua const &);

  std::string path;
  sol::state lua;
};

} // namespace Components
} // namespace HomeAutomation
