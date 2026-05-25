use std::{
    path::Path,
    sync::{
        atomic::{AtomicBool, Ordering},
        Arc,
    },
};

use anyhow::Context;
use tracing::{info, warn};

use crate::{
    config::Config,
    factory::{ProgramRegistry, Runtime},
    scheduler::{run_scheduler, FixedRateScheduler, SchedulerClock, SchedulerEvent, StdClock},
};

pub fn run_with_registry(
    config_path: impl AsRef<Path>,
    registry: &ProgramRegistry,
) -> anyhow::Result<()> {
    let config_path = config_path.as_ref();
    let config = Config::from_path(config_path)
        .with_context(|| format!("failed to load config {}", config_path.display()))?;
    run_config_with_registry(config, registry)
}

pub fn run_config_with_registry(
    config: Config,
    registry: &ProgramRegistry,
) -> anyhow::Result<()> {
    let stop_requested = install_stop_signal_handler()?;
    run_config_with_registry_until(config, registry, || {
        stop_requested.load(Ordering::SeqCst)
    })
}

fn install_stop_signal_handler() -> anyhow::Result<Arc<AtomicBool>> {
    let stop_requested = Arc::new(AtomicBool::new(false));
    {
        let stop_requested = Arc::clone(&stop_requested);
        ctrlc::set_handler(move || {
            stop_requested.store(true, Ordering::SeqCst);
        })
        .context("failed to install Ctrl-C handler")?;
    }
    Ok(stop_requested)
}

pub fn run_config_with_registry_until<Stop>(
    config: Config,
    registry: &ProgramRegistry,
    should_stop: Stop,
) -> anyhow::Result<()>
where
    Stop: FnMut() -> bool,
{
    config.validate()?;

    info!(
        "loaded {} task(s), {} input(s), {} output(s)",
        config.tasks.len(),
        config.global_vars.inputs.len(),
        config.global_vars.outputs.len()
    );

    let mut runtime = Runtime::from_config_with_registry(config, registry)?;
    let mut clock = StdClock::new();
    run_runtime_with_clock_until(&mut runtime, &mut clock, should_stop)
}

pub fn run_runtime_with_clock_until<C, Stop>(
    runtime: &mut Runtime,
    clock: &mut C,
    should_stop: Stop,
) -> anyhow::Result<()>
where
    C: SchedulerClock,
    Stop: FnMut() -> bool,
{
    runtime.init()?;
    let task_names: Vec<String> = runtime.tasks.iter().map(|task| task.name.clone()).collect();

    let start_micros = clock.now_micros();
    let mut scheduler = FixedRateScheduler::from_runtime(runtime, start_micros);

    info!("starting scheduler");
    run_scheduler(
        runtime,
        &mut scheduler,
        clock,
        should_stop,
        |events| log_scheduler_events(&task_names, events),
    )?;
    info!("scheduler stopped");

    Ok(())
}

fn log_scheduler_events(task_names: &[String], events: &[SchedulerEvent]) {
    for event in events {
        match event {
            SchedulerEvent::TaskRan { .. } => {}
            SchedulerEvent::TaskOverran {
                task_index,
                interval_micros,
                execution_micros,
            } => warn!(
                task_index,
                task = task_name(task_names, *task_index),
                interval_micros,
                execution_micros,
                "task execution exceeded configured interval"
            ),
            SchedulerEvent::TaskSkipped {
                task_index,
                skipped,
            } => warn!(
                task_index,
                task = task_name(task_names, *task_index),
                skipped,
                "task cycles skipped to realign scheduler"
            ),
            SchedulerEvent::EventBufferCapacityExceeded { capacity } => {
                warn!(capacity, "scheduler event buffer capacity exhausted")
            }
        }
    }
}

fn task_name(task_names: &[String], task_index: usize) -> &str {
    task_names
        .get(task_index)
        .map(String::as_str)
        .unwrap_or("<unknown>")
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        gv::{Gv, VarValue},
        runtime::Program,
    };

    #[derive(Default)]
    struct ManualClock {
        now_micros: u64,
    }

    impl SchedulerClock for ManualClock {
        fn now_micros(&mut self) -> u64 {
            self.now_micros
        }

        fn sleep_until_micros(&mut self, deadline_micros: u64) {
            self.now_micros = deadline_micros;
        }
    }

    struct SetOutputProgram;

    impl Program for SetOutputProgram {
        fn init(&mut self, gv: &mut Gv) -> anyhow::Result<()> {
            gv.outputs
                .insert("custom_ready".to_string(), VarValue::Bool(false));
            Ok(())
        }

        fn cycle(&mut self, gv: &mut Gv, _now_micros: u64) -> anyhow::Result<()> {
            gv.outputs
                .insert("custom_ready".to_string(), VarValue::Bool(true));
            Ok(())
        }
    }

    #[test]
    fn run_config_with_registry_runs_registered_rust_programs() {
        let mut registry = ProgramRegistry::new();
        registry.register_rust_program("SetOutput", || Box::new(SetOutputProgram));

        let config = rust_program_config();
        let mut runtime =
            Runtime::from_config_with_registry(config, &registry).expect("runtime should build");
        let mut clock = ManualClock::default();
        let mut stop_calls = 0;

        run_runtime_with_clock_until(&mut runtime, &mut clock, || {
            stop_calls += 1;
            stop_calls > 1
        })
        .expect("runtime should run");

        assert_eq!(runtime.gv.outputs["custom_ready"], VarValue::Bool(true));
    }

    #[test]
    fn run_config_with_registry_until_accepts_custom_stop_source() {
        let mut registry = ProgramRegistry::new();
        registry.register_rust_program("SetOutput", || Box::new(SetOutputProgram));
        let mut stop_calls = 0;

        run_config_with_registry_until(rust_program_config(), &registry, || {
            stop_calls += 1;
            stop_calls > 1
        })
        .expect("runtime should run");

        assert_eq!(stop_calls, 2);
    }

    fn rust_program_config() -> Config {
        toml::from_str(
            r#"
[[tasks]]
name = "main"
interval = 25000

[[tasks.programs]]
name = "SetOutput"
type = "Rust"
"#,
        )
        .expect("config should parse")
    }
}
