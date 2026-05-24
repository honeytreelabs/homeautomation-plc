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
