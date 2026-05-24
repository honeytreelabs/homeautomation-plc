use std::path::{Path, PathBuf};

use homeautomation_plc::{config::ProgramType, gv::VarValue, Config, Runtime};
use toml::{Table, Value};

fn example_path(name: &str) -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("examples")
        .join(name)
}

fn load_example(name: &str) -> Config {
    let config = Config::from_path(example_path(name)).expect("example config should parse");
    config.validate().expect("example config should validate");
    config
}

fn table<'a>(parent: &'a Table, key: &str) -> &'a Table {
    parent
        .get(key)
        .and_then(Value::as_table)
        .unwrap_or_else(|| panic!("expected '{key}' table"))
}

fn string<'a>(parent: &'a Table, key: &str) -> &'a str {
    parent
        .get(key)
        .and_then(Value::as_str)
        .unwrap_or_else(|| panic!("expected '{key}' string"))
}

#[test]
fn parses_generic_inline_example() {
    let config = load_example("generic-inline.toml");

    assert_eq!(config.tasks.len(), 1);

    let task = &config.tasks[0];
    assert_eq!(task.name, "main");
    assert_eq!(task.interval, 25_000);
    assert_eq!(task.programs.len(), 1);
    assert_eq!(task.io.len(), 1);

    let program = &task.programs[0];
    assert_eq!(program.name, "BlindLogic");
    assert_eq!(program.program_type, ProgramType::Lua);
    assert!(program.script_path.is_none());
    assert!(program
        .script
        .as_deref()
        .is_some_and(|script| script.contains("BlindConfigFromMillis")));

    let io = &task.io[0];
    assert_eq!(io.io_type, "i2c");
    assert_eq!(string(&io.settings, "bus"), "/dev/i2c-1");

    let components = table(&io.settings, "components");
    let input = table(components, "0x3b");
    assert_eq!(string(input, "type"), "pcf8574");
    assert_eq!(string(input, "direction"), "input");
    assert_eq!(string(table(input, "inputs"), "0"), "button_up");

    let output = table(components, "0x20");
    assert_eq!(string(output, "type"), "max7311");
    assert_eq!(string(output, "direction"), "output");
    assert_eq!(string(table(output, "outputs"), "1"), "blind_down");
}

#[test]
fn parses_generic_external_lua_example() {
    let config = load_example("generic.toml");

    assert_eq!(config.tasks.len(), 1);

    let task = &config.tasks[0];
    assert_eq!(task.name, "main");
    assert_eq!(task.interval, 25_000);
    assert_eq!(task.programs.len(), 1);
    assert_eq!(task.io.len(), 2);

    let program = &task.programs[0];
    assert_eq!(program.name, "LuaLogic");
    assert_eq!(program.program_type, ProgramType::Lua);
    assert!(program.script.is_none());
    assert_eq!(
        program.script_path.as_deref(),
        Some("/opt/generic_main_lualogic.lua")
    );

    let mqtt = &task.io[0];
    assert_eq!(mqtt.io_type, "mqtt");
    assert_eq!(
        string(table(&mqtt.settings, "client"), "address"),
        "tcp://localhost:1883"
    );
    assert_eq!(
        string(table(&mqtt.settings, "inputs"), "/homeautomation/light_remote"),
        "light_remote"
    );
    assert!(table(&mqtt.settings, "outputs").is_empty());

    let i2c = &task.io[1];
    assert_eq!(i2c.io_type, "i2c");
    assert_eq!(string(&i2c.settings, "bus"), "/dev/i2c-1");

    let components = table(&i2c.settings, "components");
    assert_eq!(string(table(components, "0x3b"), "type"), "pcf8574");
    assert_eq!(string(table(components, "0x20"), "type"), "max7311");
    assert_eq!(
        string(table(table(components, "0x20"), "outputs"), "4"),
        "light_a"
    );
}

#[test]
fn parses_and_runs_rpi2_lua_smoke_example() {
    let config = load_example("rpi-lua-smoke.toml");

    assert_eq!(config.tasks.len(), 1);

    let task = &config.tasks[0];
    assert_eq!(task.name, "lua_smoke");
    assert_eq!(task.interval, 1_000_000);
    assert_eq!(task.programs.len(), 1);
    assert!(task.io.is_empty());

    let program = &task.programs[0];
    assert_eq!(program.name, "LuaSmoke");
    assert_eq!(program.program_type, ProgramType::Lua);
    assert!(program.script_path.is_none());
    assert!(program
        .script
        .as_deref()
        .is_some_and(|script| script.contains("to_millis_since_start")));

    let mut runtime = Runtime::from_config(config).expect("runtime should build");
    runtime.init().expect("init should run");
    assert_eq!(runtime.gv.outputs["tick_count"], VarValue::Int(0));

    runtime.tick_once(1_234_567).expect("cycle should run");
    assert_eq!(runtime.gv.outputs["tick_count"], VarValue::Int(1));
    assert_eq!(runtime.gv.outputs["signal"], VarValue::Bool(true));
    assert_eq!(runtime.gv.outputs["rising_edge"], VarValue::Bool(true));
    assert_eq!(runtime.gv.outputs["falling_edge"], VarValue::Bool(false));
}
