use std::path::PathBuf;

use anyhow::Context;
use clap::Parser;
use homeautomation_plc::Config;

#[derive(Debug, Parser)]
#[command(author, version, about = "PLC-style cyclic runtime for home automation")]
struct Args {
    #[arg(long)]
    config: PathBuf,
}

fn main() -> anyhow::Result<()> {
    let args = Args::parse();
    let config = Config::from_path(&args.config)
        .with_context(|| format!("failed to load config {}", args.config.display()))?;

    config.validate()?;

    println!(
        "loaded {} task(s), {} input(s), {} output(s)",
        config.tasks.len(),
        config.global_vars.inputs.len(),
        config.global_vars.outputs.len()
    );

    Ok(())
}
