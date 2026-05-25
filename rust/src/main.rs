use std::path::PathBuf;

use clap::Parser;
use homeautomation_plc::{run_with_registry, ProgramRegistry};
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
    run_with_registry(&args.config, &ProgramRegistry::new())
}
