use std::{collections::BTreeMap, fs, path::Path};

use serde::Deserialize;
use thiserror::Error;

use crate::gv::VarValue;

#[derive(Debug, Deserialize)]
pub struct Config {
    pub tasks: Vec<TaskConfig>,
    #[serde(default)]
    pub global_vars: GlobalVarsConfig,
}

#[derive(Debug, Default, Deserialize)]
pub struct GlobalVarsConfig {
    #[serde(default)]
    pub inputs: BTreeMap<String, VarConfig>,
    #[serde(default)]
    pub outputs: BTreeMap<String, VarConfig>,
}

#[derive(Debug, Deserialize)]
pub struct VarConfig {
    pub init_val: VarValue,
}

#[derive(Debug, Deserialize)]
pub struct TaskConfig {
    pub name: String,
    pub interval: u64,
    #[serde(default)]
    pub programs: Vec<ProgramConfig>,
    #[serde(default)]
    pub io: Vec<IoConfig>,
}

#[derive(Debug, Deserialize)]
pub struct ProgramConfig {
    pub name: String,
    #[serde(rename = "type")]
    pub program_type: ProgramType,
    pub script: Option<String>,
    pub script_path: Option<String>,
}

#[derive(Debug, Deserialize, PartialEq, Eq)]
pub enum ProgramType {
    #[serde(rename = "Lua")]
    Lua,
    #[serde(rename = "C++")]
    Cpp,
}

#[derive(Debug, Deserialize)]
pub struct IoConfig {
    #[serde(rename = "type")]
    pub io_type: String,
    #[serde(flatten)]
    pub settings: toml::Table,
}

#[derive(Debug, Error)]
pub enum ConfigError {
    #[error("task '{task}' program '{program}' uses unsupported program type C++")]
    UnsupportedCppProgram { task: String, program: String },
    #[error("task '{task}' program '{program}' must define exactly one of script or script_path")]
    InvalidLuaScriptSource { task: String, program: String },
    #[error("task '{task}' interval must be greater than zero microseconds")]
    InvalidTaskInterval { task: String },
}

impl Config {
    pub fn from_path(path: impl AsRef<Path>) -> Result<Self, anyhow::Error> {
        let contents = fs::read_to_string(path)?;
        Ok(toml::from_str(&contents)?)
    }

    pub fn validate(&self) -> Result<(), ConfigError> {
        for task in &self.tasks {
            if task.interval == 0 {
                return Err(ConfigError::InvalidTaskInterval {
                    task: task.name.clone(),
                });
            }

            for program in &task.programs {
                if program.program_type == ProgramType::Cpp {
                    return Err(ConfigError::UnsupportedCppProgram {
                        task: task.name.clone(),
                        program: program.name.clone(),
                    });
                }

                if program.script.is_some() == program.script_path.is_some() {
                    return Err(ConfigError::InvalidLuaScriptSource {
                        task: task.name.clone(),
                        program: program.name.clone(),
                    });
                }
            }
        }

        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_minimal_inline_lua_config() {
        let config: Config = toml::from_str(
            r#"
[[tasks]]
name = "main"
interval = 25000
io = []

[[tasks.programs]]
name = "BlindLogic"
type = "Lua"
script = '''
function Init(gv) end
function Cycle(gv, now) end
'''

[global_vars.inputs.some_input]
init_val = false

[global_vars.outputs.some_output]
init_val = false
"#,
        )
        .expect("config should parse");

        assert_eq!(config.tasks.len(), 1);
        assert_eq!(config.tasks[0].name, "main");
        assert_eq!(config.tasks[0].interval, 25_000);
        assert_eq!(config.global_vars.inputs["some_input"].init_val, VarValue::Bool(false));
        assert_eq!(
            config.global_vars.outputs["some_output"].init_val,
            VarValue::Bool(false)
        );
        config.validate().expect("config should be valid");
    }

    #[test]
    fn rejects_cpp_programs() {
        let config: Config = toml::from_str(
            r#"
[[tasks]]
name = "main"
interval = 25000

[[tasks.programs]]
name = "NativeLogic"
type = "C++"
script_path = "./logic.so"

[global_vars]
inputs = {}
outputs = {}
"#,
        )
        .expect("config should parse");

        assert!(matches!(
            config.validate(),
            Err(ConfigError::UnsupportedCppProgram { .. })
        ));
    }

    #[test]
    fn rejects_missing_lua_script_source() {
        let config: Config = toml::from_str(
            r#"
[[tasks]]
name = "main"
interval = 25000

[[tasks.programs]]
name = "LuaLogic"
type = "Lua"

[global_vars]
inputs = {}
outputs = {}
"#,
        )
        .expect("config should parse");

        assert!(matches!(
            config.validate(),
            Err(ConfigError::InvalidLuaScriptSource { .. })
        ));
    }

    #[test]
    fn accepts_configs_without_global_vars_for_reference_examples() {
        let config: Config = toml::from_str(
            r#"
[[tasks]]
name = "main"
interval = 25000
io = []

[[tasks.programs]]
name = "LuaLogic"
type = "Lua"
script_path = "/opt/generic_main_lualogic.lua"
"#,
        )
        .expect("config should parse");

        assert!(config.global_vars.inputs.is_empty());
        assert!(config.global_vars.outputs.is_empty());
        config.validate().expect("config should be valid");
    }
}
