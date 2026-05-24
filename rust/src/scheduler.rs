use std::{
    thread,
    time::{Duration, Instant},
};

use crate::{factory::Runtime, runtime::Task};

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum SchedulerEvent {
    TaskRan {
        task: String,
        scheduled_micros: u64,
        started_micros: u64,
        finished_micros: u64,
    },
    TaskOverran {
        task: String,
        interval_micros: u64,
        execution_micros: u64,
    },
    TaskSkipped {
        task: String,
        skipped: u64,
    },
}

pub trait SchedulerClock {
    fn now_micros(&mut self) -> u64;

    fn sleep_until_micros(&mut self, deadline_micros: u64);
}

pub struct StdClock {
    start: Instant,
}

impl StdClock {
    pub fn new() -> Self {
        Self {
            start: Instant::now(),
        }
    }
}

impl Default for StdClock {
    fn default() -> Self {
        Self::new()
    }
}

impl SchedulerClock for StdClock {
    fn now_micros(&mut self) -> u64 {
        self.start
            .elapsed()
            .as_micros()
            .try_into()
            .unwrap_or(u64::MAX)
    }

    fn sleep_until_micros(&mut self, deadline_micros: u64) {
        let now_micros = self.now_micros();
        if deadline_micros > now_micros {
            thread::sleep(Duration::from_micros(deadline_micros - now_micros));
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct ScheduledTask {
    name: String,
    interval_micros: u64,
    next_due_micros: u64,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FixedRateScheduler {
    tasks: Vec<ScheduledTask>,
}

impl FixedRateScheduler {
    pub fn from_tasks(tasks: &[Task], start_micros: u64) -> Self {
        Self {
            tasks: tasks
                .iter()
                .map(|task| ScheduledTask {
                    name: task.name.clone(),
                    interval_micros: task.interval_micros,
                    next_due_micros: start_micros,
                })
                .collect(),
        }
    }

    pub fn from_runtime(runtime: &Runtime, start_micros: u64) -> Self {
        Self::from_tasks(&runtime.tasks, start_micros)
    }

    pub fn next_due_micros(&self) -> Option<u64> {
        self.tasks
            .iter()
            .map(|task| task.next_due_micros)
            .min()
    }

    pub fn task_next_due_micros(&self, task_index: usize) -> Option<u64> {
        self.tasks.get(task_index).map(|task| task.next_due_micros)
    }

    pub fn is_due(&self, task_index: usize, now_micros: u64) -> bool {
        self.tasks
            .get(task_index)
            .is_some_and(|task| now_micros >= task.next_due_micros)
    }

    pub fn record_task_completion(
        &mut self,
        task_index: usize,
        started_micros: u64,
        finished_micros: u64,
    ) -> anyhow::Result<Vec<SchedulerEvent>> {
        let task = self
            .tasks
            .get_mut(task_index)
            .ok_or_else(|| anyhow::anyhow!("scheduler task index {task_index} is out of bounds"))?;
        let execution_micros = finished_micros.saturating_sub(started_micros);
        let mut events = Vec::new();

        if execution_micros > task.interval_micros {
            events.push(SchedulerEvent::TaskOverran {
                task: task.name.clone(),
                interval_micros: task.interval_micros,
                execution_micros,
            });
        }

        let skipped = finished_micros
            .saturating_sub(task.next_due_micros)
            / task.interval_micros;

        if skipped > 0 {
            events.push(SchedulerEvent::TaskSkipped {
                task: task.name.clone(),
                skipped,
            });
        }

        task.next_due_micros += (skipped + 1) * task.interval_micros;

        Ok(events)
    }

    pub fn tick_due<C: SchedulerClock>(
        &mut self,
        runtime: &mut Runtime,
        clock: &mut C,
    ) -> anyhow::Result<Vec<SchedulerEvent>> {
        if self.tasks.len() != runtime.tasks.len() {
            anyhow::bail!(
                "scheduler task count {} does not match runtime task count {}",
                self.tasks.len(),
                runtime.tasks.len()
            );
        }

        let mut events = Vec::new();

        for task_index in 0..self.tasks.len() {
            let now_micros = clock.now_micros();
            if !self.is_due(task_index, now_micros) {
                continue;
            }

            let scheduled_micros = self.tasks[task_index].next_due_micros;
            let started_micros = clock.now_micros();
            runtime.tasks[task_index].tick(&mut runtime.gv, started_micros)?;
            let finished_micros = clock.now_micros();
            let task_name = self.tasks[task_index].name.clone();

            events.push(SchedulerEvent::TaskRan {
                task: task_name,
                scheduled_micros,
                started_micros,
                finished_micros,
            });
            events.extend(self.record_task_completion(
                task_index,
                started_micros,
                finished_micros,
            )?);
        }

        Ok(events)
    }
}

pub fn run_scheduler<C, Stop, Events>(
    runtime: &mut Runtime,
    scheduler: &mut FixedRateScheduler,
    clock: &mut C,
    mut should_stop: Stop,
    mut on_events: Events,
) -> anyhow::Result<()>
where
    C: SchedulerClock,
    Stop: FnMut() -> bool,
    Events: FnMut(&[SchedulerEvent]),
{
    while !should_stop() {
        let events = scheduler.tick_due(runtime, clock)?;
        on_events(&events);

        let Some(next_due_micros) = scheduler.next_due_micros() else {
            break;
        };

        let now_micros = clock.now_micros();
        if next_due_micros > now_micros {
            clock.sleep_until_micros(next_due_micros);
        }
    }

    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        factory::{ProgramRegistry, Runtime},
        gv::{Gv, VarValue},
        runtime::Program,
        Config,
    };

    struct ManualClock {
        now_micros: u64,
        sleeps: Vec<u64>,
    }

    impl SchedulerClock for ManualClock {
        fn now_micros(&mut self) -> u64 {
            self.now_micros
        }

        fn sleep_until_micros(&mut self, deadline_micros: u64) {
            self.sleeps.push(deadline_micros);
            self.now_micros = deadline_micros;
        }
    }

    struct SetOutputProgram {
        output: String,
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
            *output = VarValue::Bool(true);
            Ok(())
        }
    }

    fn runtime_with_two_tasks() -> Runtime {
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
            })
        });
        registry.register_rust_program("SetSlow", || {
            Box::new(SetOutputProgram {
                output: "slow".to_string(),
            })
        });

        Runtime::from_config_with_registry(config, &registry).expect("runtime should build")
    }

    #[test]
    fn first_cycle_is_due_immediately() {
        let mut runtime = runtime_with_two_tasks();
        runtime.init().expect("init should run");
        let mut scheduler = FixedRateScheduler::from_runtime(&runtime, 0);
        let mut clock = ManualClock {
            now_micros: 0,
            sleeps: Vec::new(),
        };

        let events = scheduler
            .tick_due(&mut runtime, &mut clock)
            .expect("scheduler should run");

        assert_eq!(
            events,
            vec![
                SchedulerEvent::TaskRan {
                    task: "fast".to_string(),
                    scheduled_micros: 0,
                    started_micros: 0,
                    finished_micros: 0,
                },
                SchedulerEvent::TaskRan {
                    task: "slow".to_string(),
                    scheduled_micros: 0,
                    started_micros: 0,
                    finished_micros: 0,
                },
            ]
        );
        assert_eq!(runtime.gv.outputs["fast"], VarValue::Bool(true));
        assert_eq!(runtime.gv.outputs["slow"], VarValue::Bool(true));
        assert_eq!(scheduler.next_due_micros(), Some(25_000));
    }

    #[test]
    fn only_due_tasks_run_after_initial_cycle() {
        let mut runtime = runtime_with_two_tasks();
        runtime.init().expect("init should run");
        let mut scheduler = FixedRateScheduler::from_runtime(&runtime, 0);
        let mut clock = ManualClock {
            now_micros: 0,
            sleeps: Vec::new(),
        };
        scheduler
            .tick_due(&mut runtime, &mut clock)
            .expect("initial cycle should run");

        clock.now_micros = 25_000;
        let events = scheduler
            .tick_due(&mut runtime, &mut clock)
            .expect("scheduler should run");

        assert_eq!(
            events,
            vec![SchedulerEvent::TaskRan {
                task: "fast".to_string(),
                scheduled_micros: 25_000,
                started_micros: 25_000,
                finished_micros: 25_000,
            }]
        );
        assert_eq!(scheduler.next_due_micros(), Some(50_000));
    }

    #[test]
    fn completion_skips_missed_cycles_and_realigns_to_raster() {
        let runtime = runtime_with_two_tasks();
        let mut scheduler = FixedRateScheduler::from_runtime(&runtime, 0);

        let events = scheduler
            .record_task_completion(0, 60_000, 70_000)
            .expect("completion should record");

        assert_eq!(
            events,
            vec![SchedulerEvent::TaskSkipped {
                task: "fast".to_string(),
                skipped: 2,
            }]
        );
        assert_eq!(scheduler.task_next_due_micros(0), Some(75_000));
    }

    #[test]
    fn completion_reports_overruns() {
        let runtime = runtime_with_two_tasks();
        let mut scheduler = FixedRateScheduler::from_runtime(&runtime, 0);

        let events = scheduler
            .record_task_completion(0, 0, 30_000)
            .expect("completion should record");

        assert_eq!(
            events,
            vec![
                SchedulerEvent::TaskOverran {
                    task: "fast".to_string(),
                    interval_micros: 25_000,
                    execution_micros: 30_000,
                },
                SchedulerEvent::TaskSkipped {
                    task: "fast".to_string(),
                    skipped: 1,
                },
            ]
        );
        assert_eq!(scheduler.task_next_due_micros(0), Some(50_000));
    }

    #[test]
    fn run_scheduler_sleeps_until_next_due_and_stops() {
        let mut runtime = runtime_with_two_tasks();
        runtime.init().expect("init should run");
        let mut scheduler = FixedRateScheduler::from_runtime(&runtime, 0);
        let mut clock = ManualClock {
            now_micros: 0,
            sleeps: Vec::new(),
        };
        let mut iterations = 0;
        let mut all_events = Vec::new();

        run_scheduler(
            &mut runtime,
            &mut scheduler,
            &mut clock,
            || {
                iterations += 1;
                iterations > 2
            },
            |events| all_events.extend_from_slice(events),
        )
        .expect("scheduler loop should run");

        assert_eq!(clock.sleeps, vec![25_000, 50_000]);
        assert_eq!(
            all_events
                .iter()
                .filter(|event| matches!(event, SchedulerEvent::TaskRan { .. }))
                .count(),
            3
        );
    }

    #[test]
    fn run_scheduler_runs_due_tasks_before_sleeping_when_late() {
        let mut runtime = runtime_with_two_tasks();
        runtime.init().expect("init should run");
        let mut scheduler = FixedRateScheduler::from_runtime(&runtime, 0);
        let mut clock = ManualClock {
            now_micros: 70_000,
            sleeps: Vec::new(),
        };
        let mut iterations = 0;
        let mut all_events = Vec::new();

        run_scheduler(
            &mut runtime,
            &mut scheduler,
            &mut clock,
            || {
                iterations += 1;
                iterations > 1
            },
            |events| all_events.extend_from_slice(events),
        )
        .expect("scheduler loop should run");

        assert!(all_events.iter().any(|event| matches!(
            event,
            SchedulerEvent::TaskSkipped {
                task,
                skipped: 2,
            } if task == "fast"
        )));
        assert_eq!(clock.sleeps, vec![75_000]);
    }
}
