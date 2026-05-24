use std::{collections::BTreeMap, fs, path::Path};

use mlua::{Function, Lua, Table, Value};
use thiserror::Error;

use crate::{
    gv::{Gv, VarValue},
    runtime::Program,
};

pub struct LuaProgram {
    lua: Lua,
}

#[derive(Debug, Error)]
pub enum LuaProgramError {
    #[error("failed to read Lua script {path}: {source}")]
    ReadScript {
        path: String,
        #[source]
        source: std::io::Error,
    },
    #[error("failed to load Lua script: {0}")]
    Load(String),
    #[error("Lua script must define function {0}")]
    MissingFunction(&'static str),
}

impl LuaProgram {
    pub fn from_inline(script: &str) -> Result<Self, LuaProgramError> {
        let lua = Lua::new();
        register_builtin_library(&lua)?;
        lua.load(script).exec().map_err(lua_load_error)?;
        require_function(&lua, "Init")?;
        require_function(&lua, "Cycle")?;
        Ok(Self { lua })
    }

    pub fn from_path(path: impl AsRef<Path>) -> Result<Self, LuaProgramError> {
        let path = path.as_ref();
        let script = fs::read_to_string(path).map_err(|source| LuaProgramError::ReadScript {
            path: path.display().to_string(),
            source,
        })?;
        Self::from_inline(&script)
    }

    fn make_gv_table(&self, gv: &Gv) -> mlua::Result<Table> {
        let table = self.lua.create_table()?;
        table.set("inputs", self.make_vars_table(&gv.inputs)?)?;
        table.set("outputs", self.make_vars_table(&gv.outputs)?)?;
        Ok(table)
    }

    fn make_vars_table(
        &self,
        vars: &std::collections::BTreeMap<String, VarValue>,
    ) -> mlua::Result<Table> {
        let table = self.lua.create_table()?;
        for (name, value) in vars {
            match value {
                VarValue::Bool(value) => table.set(name.as_str(), *value)?,
                VarValue::Int(value) => table.set(name.as_str(), *value)?,
            }
        }
        Ok(table)
    }

    fn copy_outputs_from_lua(&self, gv: &mut Gv, gv_table: Table) -> mlua::Result<()> {
        let outputs: Table = gv_table.get("outputs")?;
        let mut next_outputs = BTreeMap::new();

        for pair in outputs.pairs::<String, Value>() {
            let (name, value) = pair?;
            next_outputs.insert(name, var_value_from_lua(value)?);
        }

        gv.outputs = next_outputs;
        Ok(())
    }
}

impl Program for LuaProgram {
    fn init(&mut self, gv: &mut Gv) -> anyhow::Result<()> {
        let gv_table = self.make_gv_table(gv).map_err(lua_runtime_error)?;
        let init: Function = self.lua.globals().get("Init").map_err(lua_runtime_error)?;
        init.call::<()>(gv_table.clone())
            .map_err(lua_runtime_error)?;
        self.copy_outputs_from_lua(gv, gv_table)
            .map_err(lua_runtime_error)?;
        Ok(())
    }

    fn cycle(&mut self, gv: &mut Gv, now_micros: u64) -> anyhow::Result<()> {
        let gv_table = self.make_gv_table(gv).map_err(lua_runtime_error)?;
        let cycle: Function = self.lua.globals().get("Cycle").map_err(lua_runtime_error)?;
        let now_micros = i64::try_from(now_micros).unwrap_or(i64::MAX);
        cycle
            .call::<()>((gv_table.clone(), now_micros))
            .map_err(lua_runtime_error)?;
        self.copy_outputs_from_lua(gv, gv_table)
            .map_err(lua_runtime_error)?;
        Ok(())
    }
}

fn require_function(lua: &Lua, name: &'static str) -> Result<(), LuaProgramError> {
    match lua.globals().get::<Value>(name).map_err(lua_load_error)? {
        Value::Function(_) => Ok(()),
        _ => Err(LuaProgramError::MissingFunction(name)),
    }
}

fn register_builtin_library(lua: &Lua) -> Result<(), LuaProgramError> {
    lua.load(
        r#"
function R_TRIG(last)
  local self = { last = last or false }

  function self:execute(cur)
    cur = cur or false
    local ret = (not self.last) and cur
    self.last = cur
    return ret
  end

  return self
end

function F_TRIG(last)
  local self = { last = last or false }

  function self:execute(cur)
    cur = cur or false
    local ret = self.last and (not cur)
    self.last = cur
    return ret
  end

  return self
end

function to_millis_since_start(ts)
  return ts // 1000
end

function BlindConfigFromMillis(periodIdle, periodUp, periodDown)
  return {
    period_idle = periodIdle * 1000,
    period_up = periodUp * 1000,
    period_down = periodDown * 1000
  }
end

Blind = {}

function Blind.new(cfg)
  local self = {
    cfg = cfg,
    state = "idle",
    start = 0,
    outputs_up = false,
    outputs_down = false,
    up_trigger = R_TRIG(false),
    down_trigger = R_TRIG(false)
  }

  local function enter_idle(now, button_up, button_down)
    self.state = "idle"
    self.start = now
    self.outputs_up = false
    self.outputs_down = false
    self.up_trigger = R_TRIG(button_up)
    self.down_trigger = R_TRIG(button_down)
  end

  local function enter_up(now, button_up, button_down)
    self.state = "up"
    self.start = now
    self.outputs_up = true
    self.outputs_down = false
    self.up_trigger = R_TRIG(button_up)
    self.down_trigger = R_TRIG(button_down)
  end

  local function enter_down(now, button_up, button_down)
    self.state = "down"
    self.start = now
    self.outputs_up = false
    self.outputs_down = true
    self.up_trigger = R_TRIG(button_up)
    self.down_trigger = R_TRIG(button_down)
  end

  function self:execute(now, button_up, button_down)
    if self.state == "idle" then
      local up_triggered = self.up_trigger:execute(button_up)
      local down_triggered = self.down_trigger:execute(button_down)

      if now - self.start >= self.cfg.period_idle then
        if up_triggered then
          enter_up(now, button_up, button_down)
        elseif down_triggered then
          enter_down(now, button_up, button_down)
        end
      end
    elseif self.state == "up" then
      if now - self.start > self.cfg.period_up
          or self.up_trigger:execute(button_up)
          or self.down_trigger:execute(button_down) then
        enter_idle(now, button_up, button_down)
      end
    elseif self.state == "down" then
      if now - self.start > self.cfg.period_down
          or self.up_trigger:execute(button_up)
          or self.down_trigger:execute(button_down) then
        enter_idle(now, button_up, button_down)
      end
    end

    return self.outputs_up, self.outputs_down
  end

  return self
end
"#,
    )
    .exec()
    .map_err(lua_load_error)
}

fn lua_load_error(error: mlua::Error) -> LuaProgramError {
    LuaProgramError::Load(error.to_string())
}

fn lua_runtime_error(error: mlua::Error) -> anyhow::Error {
    anyhow::anyhow!(error.to_string())
}

fn var_value_from_lua(value: Value) -> mlua::Result<VarValue> {
    match value {
        Value::Boolean(value) => Ok(VarValue::Bool(value)),
        Value::Integer(value) => Ok(VarValue::Int(value)),
        Value::Number(value) if value.fract() == 0.0 => Ok(VarValue::Int(value as i64)),
        other => Err(mlua::Error::FromLuaConversionError {
            from: other.type_name(),
            to: "VarValue".to_string(),
            message: Some("expected boolean or integer".to_string()),
        }),
    }
}

#[cfg(test)]
mod tests {
    use std::{
        env, fs,
        path::PathBuf,
        time::{SystemTime, UNIX_EPOCH},
    };

    use super::*;

    #[test]
    fn inline_lua_init_and_cycle_update_outputs() {
        let mut program = LuaProgram::from_inline(
            r#"
function Init(gv)
  gv.outputs.light = false
end

function Cycle(gv, now)
  gv.outputs.light = gv.inputs.button
  gv.outputs.last_cycle = now
end
"#,
        )
        .expect("Lua program should load");
        let mut gv = Gv::default();
        gv.inputs
            .insert("button".to_string(), VarValue::Bool(true));
        gv.outputs
            .insert("light".to_string(), VarValue::Bool(true));

        program.init(&mut gv).expect("Init should run");
        assert_eq!(gv.outputs["light"], VarValue::Bool(false));

        program.cycle(&mut gv, 123_456).expect("Cycle should run");
        assert_eq!(gv.outputs["light"], VarValue::Bool(true));
        assert_eq!(gv.outputs["last_cycle"], VarValue::Int(123_456));
    }

    #[test]
    fn loads_lua_from_script_path() {
        let script_path = temp_script_path();
        fs::write(
            &script_path,
            r#"
function Init(gv)
  gv.outputs.from_path = true
end

function Cycle(gv, now)
end
"#,
        )
        .expect("script should be written");

        let mut program = LuaProgram::from_path(&script_path).expect("Lua program should load");
        let mut gv = Gv::default();
        program.init(&mut gv).expect("Init should run");

        assert_eq!(gv.outputs["from_path"], VarValue::Bool(true));

        fs::remove_file(script_path).expect("script should be removed");
    }

    #[test]
    fn rejects_missing_init_function() {
        let error = match LuaProgram::from_inline(
            r#"
function Cycle(gv, now)
end
"#,
        ) {
            Ok(_) => panic!("missing Init should fail"),
            Err(error) => error,
        };

        assert!(matches!(error, LuaProgramError::MissingFunction("Init")));
    }

    #[test]
    fn rejects_missing_cycle_function() {
        let error = match LuaProgram::from_inline(
            r#"
function Init(gv)
end
"#,
        ) {
            Ok(_) => panic!("missing Cycle should fail"),
            Err(error) => error,
        };

        assert!(matches!(error, LuaProgramError::MissingFunction("Cycle")));
    }

    #[test]
    fn r_trig_reports_rising_edges() {
        let mut program = LuaProgram::from_inline(
            r#"
rising = R_TRIG()

function Init(gv)
  gv.outputs.edge = false
end

function Cycle(gv, now)
  gv.outputs.edge = rising:execute(gv.inputs.signal)
end
"#,
        )
        .expect("Lua program should load");
        let mut gv = Gv::default();
        gv.inputs
            .insert("signal".to_string(), VarValue::Bool(false));

        program.init(&mut gv).expect("Init should run");
        assert_eq!(gv.outputs["edge"], VarValue::Bool(false));

        program.cycle(&mut gv, 1).expect("Cycle should run");
        assert_eq!(gv.outputs["edge"], VarValue::Bool(false));

        gv.inputs
            .insert("signal".to_string(), VarValue::Bool(true));
        program.cycle(&mut gv, 2).expect("Cycle should run");
        assert_eq!(gv.outputs["edge"], VarValue::Bool(true));

        program.cycle(&mut gv, 3).expect("Cycle should run");
        assert_eq!(gv.outputs["edge"], VarValue::Bool(false));
    }

    #[test]
    fn f_trig_reports_falling_edges() {
        let mut program = LuaProgram::from_inline(
            r#"
falling = F_TRIG(true)

function Init(gv)
  gv.outputs.edge = false
end

function Cycle(gv, now)
  gv.outputs.edge = falling:execute(gv.inputs.signal)
end
"#,
        )
        .expect("Lua program should load");
        let mut gv = Gv::default();
        gv.inputs
            .insert("signal".to_string(), VarValue::Bool(true));

        program.init(&mut gv).expect("Init should run");
        assert_eq!(gv.outputs["edge"], VarValue::Bool(false));

        program.cycle(&mut gv, 1).expect("Cycle should run");
        assert_eq!(gv.outputs["edge"], VarValue::Bool(false));

        gv.inputs
            .insert("signal".to_string(), VarValue::Bool(false));
        program.cycle(&mut gv, 2).expect("Cycle should run");
        assert_eq!(gv.outputs["edge"], VarValue::Bool(true));

        program.cycle(&mut gv, 3).expect("Cycle should run");
        assert_eq!(gv.outputs["edge"], VarValue::Bool(false));
    }

    #[test]
    fn to_millis_since_start_converts_microseconds_to_milliseconds() {
        let mut program = LuaProgram::from_inline(
            r#"
function Init(gv)
  gv.outputs.started = true
end

function Cycle(gv, now)
  gv.outputs.now_millis = to_millis_since_start(now)
end
"#,
        )
        .expect("Lua program should load");
        let mut gv = Gv::default();

        program.init(&mut gv).expect("Init should run");
        assert_eq!(gv.outputs["started"], VarValue::Bool(true));

        program.cycle(&mut gv, 123_456).expect("Cycle should run");
        assert_eq!(gv.outputs["now_millis"], VarValue::Int(123));
    }

    #[test]
    fn blind_keeps_outputs_false_when_inputs_do_not_change() {
        let mut program = blind_program();
        let mut gv = blind_gv(false, false);

        program.init(&mut gv).expect("Init should run");
        program.cycle(&mut gv, 100_000).expect("Cycle should run");
        assert_blind_outputs(&gv, false, false);

        program.cycle(&mut gv, 200_000).expect("Cycle should run");
        assert_blind_outputs(&gv, false, false);
    }

    #[test]
    fn blind_moves_up_after_idle_period_and_rising_up_button() {
        let mut program = blind_program();
        let mut gv = blind_gv(false, false);

        program.init(&mut gv).expect("Init should run");
        program.cycle(&mut gv, 600_000).expect("Cycle should run");
        assert_blind_outputs(&gv, false, false);

        gv.inputs
            .insert("button_up".to_string(), VarValue::Bool(true));
        program.cycle(&mut gv, 700_000).expect("Cycle should run");
        assert_blind_outputs(&gv, true, false);

        program.cycle(&mut gv, 800_000).expect("Cycle should run");
        assert_blind_outputs(&gv, true, false);
    }

    #[test]
    fn blind_prioritizes_up_when_both_inputs_rise() {
        let mut program = blind_program();
        let mut gv = blind_gv(false, false);

        program.init(&mut gv).expect("Init should run");
        program.cycle(&mut gv, 600_000).expect("Cycle should run");
        assert_blind_outputs(&gv, false, false);

        gv.inputs
            .insert("button_up".to_string(), VarValue::Bool(true));
        gv.inputs
            .insert("button_down".to_string(), VarValue::Bool(true));
        program.cycle(&mut gv, 700_000).expect("Cycle should run");
        assert_blind_outputs(&gv, true, false);

        program.cycle(&mut gv, 800_000).expect("Cycle should run");
        assert_blind_outputs(&gv, true, false);
    }

    #[test]
    fn blind_complete_run_matches_reference_sequence() {
        let mut program = blind_program();
        let mut gv = blind_gv(false, false);

        program.init(&mut gv).expect("Init should run");
        program.cycle(&mut gv, 600_000).expect("Cycle should run");
        assert_blind_outputs(&gv, false, false);

        gv.inputs
            .insert("button_up".to_string(), VarValue::Bool(true));
        program.cycle(&mut gv, 700_000).expect("Cycle should run");
        assert_blind_outputs(&gv, true, false);

        program.cycle(&mut gv, 800_000).expect("Cycle should run");
        assert_blind_outputs(&gv, true, false);

        gv.inputs
            .insert("button_up".to_string(), VarValue::Bool(false));
        program.cycle(&mut gv, 900_000).expect("Cycle should run");
        assert_blind_outputs(&gv, true, false);

        gv.inputs
            .insert("button_up".to_string(), VarValue::Bool(true));
        program.cycle(&mut gv, 1_000_000).expect("Cycle should run");
        assert_blind_outputs(&gv, false, false);

        program.cycle(&mut gv, 1_100_000).expect("Cycle should run");
        assert_blind_outputs(&gv, false, false);

        gv.inputs
            .insert("button_up".to_string(), VarValue::Bool(false));
        program.cycle(&mut gv, 1_200_000).expect("Cycle should run");
        assert_blind_outputs(&gv, false, false);

        gv.inputs
            .insert("button_up".to_string(), VarValue::Bool(true));
        program.cycle(&mut gv, 1_300_000).expect("Cycle should run");
        assert_blind_outputs(&gv, false, false);

        gv.inputs
            .insert("button_up".to_string(), VarValue::Bool(false));
        gv.inputs
            .insert("button_down".to_string(), VarValue::Bool(true));
        program.cycle(&mut gv, 1_600_000).expect("Cycle should run");
        assert_blind_outputs(&gv, false, true);

        gv.inputs
            .insert("button_up".to_string(), VarValue::Bool(true));
        gv.inputs
            .insert("button_down".to_string(), VarValue::Bool(false));
        program
            .cycle(&mut gv, 31_700_000)
            .expect("Cycle should run");
        assert_blind_outputs(&gv, false, false);
    }

    #[test]
    fn blind_instances_keep_independent_state() {
        let mut program = LuaProgram::from_inline(
            r#"
local blind_1
local blind_2

function Init(gv)
  local cfg = BlindConfigFromMillis(500, 30000, 30000)
  blind_1 = Blind.new(cfg)
  blind_2 = Blind.new(cfg)
end

function Cycle(gv, now)
  gv.outputs.blind_1_up, gv.outputs.blind_1_down =
    blind_1:execute(now, gv.inputs.button_1_up, gv.inputs.button_1_down)
  gv.outputs.blind_2_up, gv.outputs.blind_2_down =
    blind_2:execute(now, gv.inputs.button_2_up, gv.inputs.button_2_down)
end
"#,
        )
        .expect("Lua program should load");
        let mut gv = Gv::default();
        gv.inputs
            .insert("button_1_up".to_string(), VarValue::Bool(false));
        gv.inputs
            .insert("button_1_down".to_string(), VarValue::Bool(false));
        gv.inputs
            .insert("button_2_up".to_string(), VarValue::Bool(false));
        gv.inputs
            .insert("button_2_down".to_string(), VarValue::Bool(false));

        program.init(&mut gv).expect("Init should run");
        program.cycle(&mut gv, 600_000).expect("Cycle should run");

        gv.inputs
            .insert("button_1_up".to_string(), VarValue::Bool(true));
        program.cycle(&mut gv, 700_000).expect("Cycle should run");
        assert_eq!(gv.outputs["blind_1_up"], VarValue::Bool(true));
        assert_eq!(gv.outputs["blind_1_down"], VarValue::Bool(false));
        assert_eq!(gv.outputs["blind_2_up"], VarValue::Bool(false));
        assert_eq!(gv.outputs["blind_2_down"], VarValue::Bool(false));

        gv.inputs
            .insert("button_2_down".to_string(), VarValue::Bool(true));
        program.cycle(&mut gv, 1_200_000).expect("Cycle should run");
        assert_eq!(gv.outputs["blind_1_up"], VarValue::Bool(true));
        assert_eq!(gv.outputs["blind_1_down"], VarValue::Bool(false));
        assert_eq!(gv.outputs["blind_2_up"], VarValue::Bool(false));
        assert_eq!(gv.outputs["blind_2_down"], VarValue::Bool(true));
    }

    fn blind_program() -> LuaProgram {
        LuaProgram::from_inline(
            r#"
local blind

function Init(gv)
  blind = Blind.new(BlindConfigFromMillis(500, 30000, 30000))
  gv.outputs.blind_up = false
  gv.outputs.blind_down = false
end

function Cycle(gv, now)
  gv.outputs.blind_up, gv.outputs.blind_down =
    blind:execute(now, gv.inputs.button_up, gv.inputs.button_down)
end
"#,
        )
        .expect("Lua program should load")
    }

    fn blind_gv(button_up: bool, button_down: bool) -> Gv {
        let mut gv = Gv::default();
        gv.inputs
            .insert("button_up".to_string(), VarValue::Bool(button_up));
        gv.inputs
            .insert("button_down".to_string(), VarValue::Bool(button_down));
        gv
    }

    fn assert_blind_outputs(gv: &Gv, up: bool, down: bool) {
        assert_eq!(gv.outputs["blind_up"], VarValue::Bool(up));
        assert_eq!(gv.outputs["blind_down"], VarValue::Bool(down));
    }

    fn temp_script_path() -> PathBuf {
        let nanos = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .expect("system time should be after epoch")
            .as_nanos();
        env::temp_dir().join(format!(
            "homeautomation-plc-lua-test-{}-{nanos}.lua",
            std::process::id()
        ))
    }
}
