use thiserror::Error;

use crate::{
    config::{Config, ProgramType},
    gv::Gv,
    runtime::Task,
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
    #[error("task '{task}' IO backend '{io_type}' is not implemented yet")]
    IoBackendNotImplemented { task: String, io_type: String },
}

impl Runtime {
    pub fn from_config(config: Config) -> Result<Self, RuntimeFactoryError> {
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
            for program in &task_config.programs {
                if program.program_type == ProgramType::Lua {
                    return Err(RuntimeFactoryError::LuaProgramNotImplemented {
                        task: task_config.name.clone(),
                        program: program.name.clone(),
                    });
                }

                unreachable!("Config::validate rejects C++ programs");
            }

            if let Some(io) = task_config.io.first() {
                return Err(RuntimeFactoryError::IoBackendNotImplemented {
                    task: task_config.name.clone(),
                    io_type: io.io_type.clone(),
                });
            }

            tasks.push(Task::new(task_config.name, task_config.interval));
        }

        Ok(Self { gv, tasks })
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::gv::VarValue;

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
