use std::collections::BTreeMap;

use thiserror::Error;

use crate::{
    config::{Config, ProgramType},
    gv::Gv,
    i2c::I2cIoConfig,
    lua::LuaProgram,
    mqtt::MqttIoConfig,
    runtime::{Program, Task},
};

pub struct Runtime {
    pub gv: Gv,
    pub tasks: Vec<Task>,
}

#[derive(Debug, Error)]
pub enum RuntimeFactoryError {
    #[error(transparent)]
    InvalidConfig(#[from] crate::config::ConfigError),
    #[error("task '{task}' program '{program}' Lua program could not be created: {source}")]
    LuaProgramBuild {
        task: String,
        program: String,
        #[source]
        source: crate::lua::LuaProgramError,
    },
    #[error("task '{task}' program '{program}' is not registered")]
    RustProgramNotRegistered { task: String, program: String },
    #[error("task '{task}' IO backend '{io_type}' is not implemented yet")]
    IoBackendNotImplemented { task: String, io_type: String },
    #[error("task '{task}' IO backend '{io_type}' config is invalid: {source}")]
    InvalidIoConfig {
        task: String,
        io_type: String,
        #[source]
        source: toml::de::Error,
    },
    #[error("task '{task}' IO backend '{io_type}' could not be created: {source}")]
    MqttIoBuild {
        task: String,
        io_type: String,
        #[source]
        source: crate::mqtt::MqttConfigError,
    },
    #[error("task '{task}' IO backend '{io_type}' could not be created: {source}")]
    I2cIoBuild {
        task: String,
        io_type: String,
        #[source]
        source: crate::i2c::I2cConfigError,
    },
}

#[derive(Default)]
pub struct ProgramRegistry {
    rust_programs: BTreeMap<String, Box<dyn Fn() -> Box<dyn Program>>>,
}

impl ProgramRegistry {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn register_rust_program(
        &mut self,
        name: impl Into<String>,
        factory: impl Fn() -> Box<dyn Program> + 'static,
    ) {
        self.rust_programs.insert(name.into(), Box::new(factory));
    }

    fn rust_program_factory(&self, name: &str) -> Option<&dyn Fn() -> Box<dyn Program>> {
        self.rust_programs.get(name).map(Box::as_ref)
    }
}

impl Runtime {
    pub fn from_config(config: Config) -> Result<Self, RuntimeFactoryError> {
        Self::from_config_with_registry(config, &ProgramRegistry::default())
    }

    pub fn from_config_with_registry(
        config: Config,
        registry: &ProgramRegistry,
    ) -> Result<Self, RuntimeFactoryError> {
        config.validate()?;

        let gv = Gv {
            inputs: config
                .global_vars
                .inputs
                .into_iter()
                .map(|(name, var)| (name, var.init_val))
                .collect(),
            outputs: config
                .global_vars
                .outputs
                .into_iter()
                .map(|(name, var)| (name, var.init_val))
                .collect(),
        };

        let mut tasks = Vec::with_capacity(config.tasks.len());

        for task_config in config.tasks {
            let mut task = Task::new(task_config.name, task_config.interval);

            for program in &task_config.programs {
                match program.program_type {
                    ProgramType::Lua => {
                        let lua_program = if let Some(script) = &program.script {
                            LuaProgram::from_inline(script)
                        } else if let Some(script_path) = &program.script_path {
                            LuaProgram::from_path(script_path)
                        } else {
                            unreachable!("Config::validate requires a Lua script source")
                        }
                        .map_err(|source| RuntimeFactoryError::LuaProgramBuild {
                            task: task.name.clone(),
                            program: program.name.clone(),
                            source,
                        })?;

                        task.add_program(Box::new(lua_program));
                    }
                    ProgramType::Rust => {
                        let factory = registry.rust_program_factory(&program.name).ok_or_else(
                            || RuntimeFactoryError::RustProgramNotRegistered {
                                task: task.name.clone(),
                                program: program.name.clone(),
                            },
                        )?;

                        task.add_program(factory());
                    }
                    ProgramType::Cpp => unreachable!("Config::validate rejects C++ programs"),
                }
            }

            for io in task_config.io {
                match io.io_type.as_str() {
                    "mqtt" => {
                        let io_type = io.io_type;
                        let mqtt_config: MqttIoConfig =
                            toml::Value::Table(io.settings).try_into().map_err(|source| {
                                RuntimeFactoryError::InvalidIoConfig {
                                    task: task.name.clone(),
                                    io_type: io_type.clone(),
                                    source,
                                }
                            })?;
                        let mqtt_io = mqtt_config.into_io().map_err(|source| {
                            RuntimeFactoryError::MqttIoBuild {
                                task: task.name.clone(),
                                io_type: io_type.clone(),
                                source,
                            }
                        })?;

                        task.add_io(Box::new(mqtt_io));
                    }
                    "i2c" => {
                        let io_type = io.io_type;
                        let i2c_config: I2cIoConfig =
                            toml::Value::Table(io.settings).try_into().map_err(|source| {
                                RuntimeFactoryError::InvalidIoConfig {
                                    task: task.name.clone(),
                                    io_type: io_type.clone(),
                                    source,
                                }
                            })?;
                        let i2c_io =
                            i2c_config
                                .into_io()
                                .map_err(|source| RuntimeFactoryError::I2cIoBuild {
                                    task: task.name.clone(),
                                    io_type: io_type.clone(),
                                    source,
                                })?;

                        task.add_io(Box::new(i2c_io));
                    }
                    _ => {
                        return Err(RuntimeFactoryError::IoBackendNotImplemented {
                            task: task.name.clone(),
                            io_type: io.io_type,
                        });
                    }
                }
            }

            tasks.push(task);
        }

        Ok(Self { gv, tasks })
    }

    pub fn init(&mut self) -> anyhow::Result<()> {
        for task in &mut self.tasks {
            task.init(&mut self.gv)?;
        }

        Ok(())
    }

    pub fn tick_once(&mut self, now_micros: u64) -> anyhow::Result<()> {
        for task in &mut self.tasks {
            task.tick(&mut self.gv, now_micros)?;
        }

        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::gv::VarValue;

    struct CopyInputProgram;

    impl Program for CopyInputProgram {
        fn init(&mut self, gv: &mut Gv) -> anyhow::Result<()> {
            gv.outputs
                .insert("initialized".to_string(), VarValue::Bool(true));
            Ok(())
        }

        fn cycle(&mut self, gv: &mut Gv, now_micros: u64) -> anyhow::Result<()> {
            let input = gv
                .inputs
                .get("button")
                .cloned()
                .unwrap_or(VarValue::Bool(false));

            gv.outputs.insert("light".to_string(), input);
            gv.outputs
                .insert("last_cycle".to_string(), VarValue::Int(now_micros as i64));
            Ok(())
        }
    }

    struct SetOutputProgram {
        output: String,
        value: VarValue,
    }

    impl Program for SetOutputProgram {
        fn init(&mut self, gv: &mut Gv) -> anyhow::Result<()> {
            gv.outputs
                .insert(self.output.clone(), VarValue::Bool(false));
            Ok(())
        }

        fn cycle(&mut self, gv: &mut Gv, _now_micros: u64) -> anyhow::Result<()> {
            let output = gv
                .outputs
                .get_mut(&self.output)
                .ok_or_else(|| anyhow::anyhow!("output '{}' was not initialized", self.output))?;

            *output = self.value.clone();
            Ok(())
        }
    }

    #[test]
    fn builds_gv_and_task_skeleton_from_config_without_programs_or_io() {
        let config: Config = toml::from_str(
            r#"
[[tasks]]
name = "main"
interval = 25000

[[tasks]]
name = "slow"
interval = 1000000

[global_vars.inputs.button]
init_val = false

[global_vars.inputs.counter]
init_val = 3

[global_vars.outputs.light]
init_val = false
"#,
        )
        .expect("config should parse");

        let runtime = Runtime::from_config(config).expect("runtime should build");

        assert_eq!(runtime.gv.inputs["button"], VarValue::Bool(false));
        assert_eq!(runtime.gv.inputs["counter"], VarValue::Int(3));
        assert_eq!(runtime.gv.outputs["light"], VarValue::Bool(false));

        assert_eq!(runtime.tasks.len(), 2);
        assert_eq!(runtime.tasks[0].name, "main");
        assert_eq!(runtime.tasks[0].interval_micros, 25_000);
        assert_eq!(runtime.tasks[0].program_count(), 0);
        assert_eq!(runtime.tasks[0].io_count(), 0);
        assert_eq!(runtime.tasks[1].name, "slow");
        assert_eq!(runtime.tasks[1].interval_micros, 1_000_000);
    }

    #[test]
    fn attaches_inline_lua_programs_to_tasks() {
        let config: Config = toml::from_str(
            r#"
[[tasks]]
name = "main"
interval = 25000

[[tasks.programs]]
name = "Logic"
type = "Lua"
script = '''
function Init(gv)
  gv.outputs.light = false
end

function Cycle(gv, now)
  gv.outputs.light = gv.inputs.button
  gv.outputs.last_cycle = now
end
'''

[global_vars.inputs.button]
init_val = true

[global_vars.outputs.light]
init_val = true
"#,
        )
        .expect("config should parse");

        let mut runtime = Runtime::from_config(config).expect("runtime should build");

        assert_eq!(runtime.tasks.len(), 1);
        assert_eq!(runtime.tasks[0].program_count(), 1);

        runtime.init().expect("init should run");
        assert_eq!(runtime.gv.outputs["light"], VarValue::Bool(false));

        runtime.tick_once(123_456).expect("cycle should run");
        assert_eq!(runtime.gv.outputs["light"], VarValue::Bool(true));
        assert_eq!(runtime.gv.outputs["last_cycle"], VarValue::Int(123_456));
    }

    #[test]
    fn rejects_invalid_lua_programs() {
        let config: Config = toml::from_str(
            r#"
[[tasks]]
name = "main"
interval = 25000

[[tasks.programs]]
name = "Logic"
type = "Lua"
script = "function Init(gv) end"
"#,
        )
        .expect("config should parse");

        assert!(matches!(
            Runtime::from_config(config),
            Err(RuntimeFactoryError::LuaProgramBuild { .. })
        ));
    }

    #[test]
    fn attaches_registered_rust_programs_to_tasks() {
        let config: Config = toml::from_str(
            r#"
[[tasks]]
name = "main"
interval = 25000

[[tasks.programs]]
name = "CopyInput"
type = "Rust"

[global_vars.inputs.button]
init_val = true

[global_vars.outputs.light]
init_val = false
"#,
        )
        .expect("config should parse");

        let mut registry = ProgramRegistry::new();
        registry.register_rust_program("CopyInput", || Box::new(CopyInputProgram));

        let mut runtime =
            Runtime::from_config_with_registry(config, &registry).expect("runtime should build");

        assert_eq!(runtime.tasks.len(), 1);
        assert_eq!(runtime.tasks[0].program_count(), 1);

        runtime.init().expect("init should run");
        assert_eq!(
            runtime.gv.outputs["initialized"],
            VarValue::Bool(true)
        );

        runtime.tick_once(123_456).expect("cycle should run");
        assert_eq!(runtime.gv.outputs["light"], VarValue::Bool(true));
        assert_eq!(runtime.gv.outputs["last_cycle"], VarValue::Int(123_456));
    }

    #[test]
    fn rejects_unregistered_rust_programs() {
        let config: Config = toml::from_str(
            r#"
[[tasks]]
name = "main"
interval = 25000

[[tasks.programs]]
name = "Missing"
type = "Rust"
"#,
        )
        .expect("config should parse");

        assert!(matches!(
            Runtime::from_config_with_registry(config, &ProgramRegistry::new()),
            Err(RuntimeFactoryError::RustProgramNotRegistered { .. })
        ));
    }

    #[test]
    fn tick_once_runs_all_tasks() {
        let config: Config = toml::from_str(
            r#"
[[tasks]]
name = "fast"
interval = 25000

[[tasks.programs]]
name = "SetFast"
type = "Rust"

[[tasks]]
name = "slow"
interval = 1000000

[[tasks.programs]]
name = "SetSlow"
type = "Rust"

"#,
        )
        .expect("config should parse");

        let mut registry = ProgramRegistry::new();
        registry.register_rust_program("SetFast", || {
            Box::new(SetOutputProgram {
                output: "fast".to_string(),
                value: VarValue::Bool(true),
            })
        });
        registry.register_rust_program("SetSlow", || {
            Box::new(SetOutputProgram {
                output: "slow".to_string(),
                value: VarValue::Bool(true),
            })
        });

        let mut runtime =
            Runtime::from_config_with_registry(config, &registry).expect("runtime should build");

        runtime.init().expect("init should run");

        assert_eq!(runtime.gv.outputs["fast"], VarValue::Bool(false));
        assert_eq!(runtime.gv.outputs["slow"], VarValue::Bool(false));

        runtime.tick_once(123_456).expect("cycle should run");

        assert_eq!(runtime.gv.outputs["fast"], VarValue::Bool(true));
        assert_eq!(runtime.gv.outputs["slow"], VarValue::Bool(true));
    }

    #[test]
    fn attaches_mqtt_io_to_tasks() {
        let config: Config = toml::from_str(
            r#"
[[tasks]]
name = "main"
interval = 25000

[[tasks.io]]
type = "mqtt"
payload_codec = "binary-bool"

[tasks.io.client]
address = "tcp://localhost:1883"
client_id = "test-client"

[tasks.io.inputs]
"/input" = "button"

[tasks.io.outputs]
"/output" = "light"
"#,
        )
        .expect("config should parse");

        let runtime = Runtime::from_config(config).expect("runtime should build");

        assert_eq!(runtime.tasks.len(), 1);
        assert_eq!(runtime.tasks[0].io_count(), 1);
    }

    #[test]
    fn attaches_i2c_io_to_tasks() {
        let config: Config = toml::from_str(
            r#"
[[tasks]]
name = "main"
interval = 25000

[[tasks.io]]
type = "i2c"
bus = "/dev/i2c-1"

[tasks.io.components."0x3b"]
type = "pcf8574"
direction = "input"
inputs = { 0 = "button" }

[tasks.io.components."0x20"]
type = "max7311"
direction = "output"
outputs = { 0 = "light" }
"#,
        )
        .expect("config should parse");

        let runtime = Runtime::from_config(config).expect("runtime should build");

        assert_eq!(runtime.tasks.len(), 1);
        assert_eq!(runtime.tasks[0].io_count(), 1);
    }

    #[test]
    fn rejects_unknown_io_backends() {
        let config: Config = toml::from_str(
            r#"
[[tasks]]
name = "main"
interval = 25000

[[tasks.io]]
type = "modbus-rtu"
"#,
        )
        .expect("config should parse");

        assert!(matches!(
            Runtime::from_config(config),
            Err(RuntimeFactoryError::IoBackendNotImplemented { .. })
        ));
    }

    #[test]
    fn rejects_invalid_mqtt_io_config() {
        let config: Config = toml::from_str(
            r#"
[[tasks]]
name = "main"
interval = 25000

[[tasks.io]]
type = "mqtt"
"#,
        )
        .expect("config should parse");

        assert!(matches!(
            Runtime::from_config(config),
            Err(RuntimeFactoryError::InvalidIoConfig { .. })
        ));
    }
}
