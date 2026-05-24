use std::collections::BTreeMap;

use thiserror::Error;

use crate::{
    config::{Config, ProgramType},
    gv::Gv,
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
    #[error("task '{task}' program '{program}' type Lua is not implemented yet")]
    LuaProgramNotImplemented { task: String, program: String },
    #[error("task '{task}' program '{program}' is not registered")]
    RustProgramNotRegistered { task: String, program: String },
    #[error("task '{task}' IO backend '{io_type}' is not implemented yet")]
    IoBackendNotImplemented { task: String, io_type: String },
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
                        return Err(RuntimeFactoryError::LuaProgramNotImplemented {
                            task: task.name.clone(),
                            program: program.name.clone(),
                        });
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

            if let Some(io) = task_config.io.first() {
                return Err(RuntimeFactoryError::IoBackendNotImplemented {
                    task: task.name.clone(),
                    io_type: io.io_type.clone(),
                });
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
    fn rejects_lua_programs_until_lua_runtime_is_implemented() {
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
            Err(RuntimeFactoryError::LuaProgramNotImplemented { .. })
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
    fn rejects_io_until_backends_are_implemented() {
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
            Err(RuntimeFactoryError::IoBackendNotImplemented { .. })
        ));
    }
}
