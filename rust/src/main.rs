use std::{
    path::PathBuf,
    sync::{
        atomic::{AtomicBool, Ordering},
        Arc,
    },
};

use anyhow::Context;
use clap::Parser;
use homeautomation_plc::{
    scheduler::{run_scheduler, FixedRateScheduler, SchedulerClock, SchedulerEvent, StdClock},
    Config, Runtime,
};
use tracing::{info, warn};
use tracing_subscriber::EnvFilter;

#[derive(Debug, Parser)]
#[command(author, version, about = "PLC-style cyclic runtime for home automation")]
struct Args {
    #[arg(long)]
    config: PathBuf,
}

fn main() -> anyhow::Result<()> {
    let env_filter =
        EnvFilter::try_from_default_env().unwrap_or_else(|_| EnvFilter::new("info"));
    tracing_subscriber::fmt()
        .with_env_filter(env_filter)
        .init();

    let args = Args::parse();
    let config = Config::from_path(&args.config)
        .with_context(|| format!("failed to load config {}", args.config.display()))?;

    config.validate()?;

    info!(
        "loaded {} task(s), {} input(s), {} output(s)",
        config.tasks.len(),
        config.global_vars.inputs.len(),
        config.global_vars.outputs.len()
    );

    let mut runtime = Runtime::from_config(config)?;
    runtime.init()?;
    let task_names: Vec<String> = runtime.tasks.iter().map(|task| task.name.clone()).collect();

    let mut clock = StdClock::new();
    let start_micros = clock.now_micros();
    let mut scheduler = FixedRateScheduler::from_runtime(&runtime, start_micros);
    let stop_requested = Arc::new(AtomicBool::new(false));

    {
        let stop_requested = Arc::clone(&stop_requested);
        ctrlc::set_handler(move || {
            stop_requested.store(true, Ordering::SeqCst);
        })
        .context("failed to install Ctrl-C handler")?;
    }

    info!("starting scheduler");
    run_scheduler(
        &mut runtime,
        &mut scheduler,
        &mut clock,
        || stop_requested.load(Ordering::SeqCst),
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
