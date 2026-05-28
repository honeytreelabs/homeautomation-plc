use std::{
    collections::BTreeMap,
    fs::{File, OpenOptions},
    io::{Read, Write},
    os::{
        fd::AsRawFd,
        raw::{c_int, c_ulong},
    },
};

use serde::Deserialize;
use thiserror::Error;

use crate::{
    gv::{Gv, VarValue},
    runtime::TaskIo,
};

const I2C_SLAVE: c_ulong = 0x0703;

extern "C" {
    fn ioctl(fd: c_int, request: c_ulong, ...) -> c_int;
}

pub trait I2cBus {
    fn open(&mut self) -> anyhow::Result<()>;
    fn read(&mut self, address: u8, data: &mut [u8]) -> anyhow::Result<()>;
    fn write(&mut self, address: u8, data: &[u8]) -> anyhow::Result<()>;
    fn close(&mut self) -> anyhow::Result<()>;
}

pub struct LinuxI2cBus {
    path: String,
    file: Option<File>,
}

impl LinuxI2cBus {
    pub fn new(path: impl Into<String>) -> Self {
        Self {
            path: path.into(),
            file: None,
        }
    }

    fn file_mut(&mut self) -> anyhow::Result<&mut File> {
        self.file
            .as_mut()
            .ok_or_else(|| anyhow::anyhow!("I2C bus '{}' is not open", self.path))
    }

    fn set_address(&mut self, address: u8) -> anyhow::Result<()> {
        let fd = self.file_mut()?.as_raw_fd();
        let rc = unsafe { ioctl(fd, I2C_SLAVE, c_ulong::from(address)) };
        if rc < 0 {
            anyhow::bail!("failed to set I2C address 0x{address:02x} on {}", self.path);
        }
        Ok(())
    }
}

impl I2cBus for LinuxI2cBus {
    fn open(&mut self) -> anyhow::Result<()> {
        self.file = Some(OpenOptions::new().read(true).write(true).open(&self.path)?);
        Ok(())
    }

    fn read(&mut self, address: u8, data: &mut [u8]) -> anyhow::Result<()> {
        self.set_address(address)?;
        self.file_mut()?.read_exact(data)?;
        Ok(())
    }

    fn write(&mut self, address: u8, data: &[u8]) -> anyhow::Result<()> {
        self.set_address(address)?;
        self.file_mut()?.write_all(data)?;
        Ok(())
    }

    fn close(&mut self) -> anyhow::Result<()> {
        self.file = None;
        Ok(())
    }
}

#[derive(Debug, Deserialize)]
pub struct I2cIoConfig {
    pub bus: String,
    pub components: BTreeMap<String, I2cComponentConfig>,
}

impl I2cIoConfig {
    pub fn into_io(self) -> Result<I2cIo<LinuxI2cBus>, I2cConfigError> {
        let mut components = Vec::with_capacity(self.components.len());
        for (address, component) in self.components {
            components.push(component.into_component(parse_address(&address)?)?);
        }

        Ok(I2cIo::new(LinuxI2cBus::new(self.bus), components))
    }
}

#[derive(Debug, Deserialize)]
pub struct I2cComponentConfig {
    #[serde(rename = "type")]
    pub component_type: String,
    pub direction: I2cDirection,
    #[serde(default)]
    pub inputs: BTreeMap<String, String>,
    #[serde(default)]
    pub outputs: BTreeMap<String, String>,
}

impl I2cComponentConfig {
    fn into_component(self, address: u8) -> Result<I2cComponent, I2cConfigError> {
        match (self.component_type.as_str(), self.direction) {
            ("pcf8574", I2cDirection::Input) => Ok(I2cComponent::Pcf8574Input {
                device: Pcf8574Input::new(address),
                inputs: parse_pin_map(self.inputs)?,
            }),
            ("pcf8574", I2cDirection::Output) => Ok(I2cComponent::Pcf8574Output {
                device: Pcf8574Output::new(address),
                outputs: parse_pin_map(self.outputs)?,
            }),
            ("max7311", I2cDirection::Output) => Ok(I2cComponent::Max7311Output {
                device: Max7311Output::new(address),
                outputs: parse_pin_map(self.outputs)?,
            }),
            ("max7311", I2cDirection::Input) => {
                Err(I2cConfigError::UnsupportedComponentDirection {
                    component_type: self.component_type,
                    direction: I2cDirection::Input,
                })
            }
            _ => Err(I2cConfigError::UnknownComponentType(self.component_type)),
        }
    }
}

#[derive(Debug, Clone, Copy, Deserialize, PartialEq, Eq)]
#[serde(rename_all = "lowercase")]
pub enum I2cDirection {
    Input,
    Output,
}

#[derive(Debug, Error, PartialEq, Eq)]
pub enum I2cConfigError {
    #[error("invalid I2C component address '{0}'")]
    InvalidAddress(String),
    #[error("invalid I2C component pin '{0}'")]
    InvalidPin(String),
    #[error("unknown I2C component type '{0}'")]
    UnknownComponentType(String),
    #[error("I2C component type '{component_type}' does not support direction {direction:?}")]
    UnsupportedComponentDirection {
        component_type: String,
        direction: I2cDirection,
    },
}

fn parse_address(address: &str) -> Result<u8, I2cConfigError> {
    let normalized = address.strip_prefix("0x").unwrap_or(address);
    u8::from_str_radix(normalized, 16)
        .map_err(|_| I2cConfigError::InvalidAddress(address.to_string()))
}

fn parse_pin(pin: &str) -> Result<u8, I2cConfigError> {
    let pin = pin
        .parse::<u8>()
        .map_err(|_| I2cConfigError::InvalidPin(pin.to_string()))?;
    if pin > 7 {
        return Err(I2cConfigError::InvalidPin(pin.to_string()));
    }
    Ok(pin)
}

fn parse_pin_map(
    pin_map: BTreeMap<String, String>,
) -> Result<BTreeMap<u8, String>, I2cConfigError> {
    pin_map
        .into_iter()
        .map(|(pin, var_name)| Ok((parse_pin(&pin)?, var_name)))
        .collect()
}

fn bit_get(value: u8, pos: u8) -> bool {
    value & (1 << pos) != 0
}

fn bit_set(value: u8, pos: u8, bit: bool) -> u8 {
    if bit {
        value | (1 << pos)
    } else {
        value & !(1 << pos)
    }
}

pub struct Pcf8574Input {
    address: u8,
    inputs: u8,
}

impl Pcf8574Input {
    pub fn new(address: u8) -> Self {
        Self {
            address,
            inputs: 0x00,
        }
    }

    pub fn init(&mut self, bus: &mut dyn I2cBus) -> anyhow::Result<()> {
        self.read(bus)
    }

    pub fn read(&mut self, bus: &mut dyn I2cBus) -> anyhow::Result<()> {
        let mut data = [0x00];
        bus.read(self.address, &mut data)?;
        self.inputs = !data[0];
        Ok(())
    }

    pub fn get_input(&self, pos: u8) -> bool {
        bit_get(self.inputs, pos)
    }
}

pub struct Pcf8574Output {
    address: u8,
    outputs: u8,
    last_outputs: u8,
}

impl Pcf8574Output {
    pub fn new(address: u8) -> Self {
        Self {
            address,
            outputs: 0x00,
            last_outputs: 0x00,
        }
    }

    pub fn init(&mut self, bus: &mut dyn I2cBus) -> anyhow::Result<()> {
        let mut data = [0x00];
        bus.read(self.address, &mut data)?;
        self.outputs = !data[0];
        Ok(())
    }

    pub fn set_output(&mut self, pos: u8, value: bool) {
        self.outputs = bit_set(self.outputs, pos, value);
    }

    pub fn write(&mut self, bus: &mut dyn I2cBus) -> anyhow::Result<()> {
        if self.outputs == self.last_outputs {
            return Ok(());
        }

        bus.write(self.address, &[!self.outputs])?;
        self.last_outputs = self.outputs;
        Ok(())
    }
}

pub struct Max7311Output {
    address: u8,
    outputs: u8,
    last_outputs: u8,
}

impl Max7311Output {
    const REGISTER_OUTPUT_PORT_1: u8 = 0x02;
    const REGISTER_PORT_1_CONFIGURATION: u8 = 0x06;

    pub fn new(address: u8) -> Self {
        Self {
            address,
            outputs: 0x00,
            last_outputs: 0x00,
        }
    }

    pub fn init(&mut self, bus: &mut dyn I2cBus) -> anyhow::Result<()> {
        bus.write(self.address, &[Self::REGISTER_PORT_1_CONFIGURATION, 0x00])?;
        bus.write(self.address, &[Self::REGISTER_OUTPUT_PORT_1])?;

        let mut data = [0x00];
        bus.read(self.address, &mut data)?;
        self.outputs = !data[0];
        Ok(())
    }

    pub fn set_output(&mut self, pos: u8, value: bool) {
        self.outputs = bit_set(self.outputs, pos, value);
    }

    pub fn write(&mut self, bus: &mut dyn I2cBus) -> anyhow::Result<()> {
        if self.outputs == self.last_outputs {
            return Ok(());
        }

        bus.write(self.address, &[Self::REGISTER_OUTPUT_PORT_1, !self.outputs])?;
        self.last_outputs = self.outputs;
        Ok(())
    }
}

pub enum I2cComponent {
    Pcf8574Input {
        device: Pcf8574Input,
        inputs: BTreeMap<u8, String>,
    },
    Pcf8574Output {
        device: Pcf8574Output,
        outputs: BTreeMap<u8, String>,
    },
    Max7311Output {
        device: Max7311Output,
        outputs: BTreeMap<u8, String>,
    },
}

impl I2cComponent {
    fn init(&mut self, bus: &mut dyn I2cBus, gv: &mut Gv) -> anyhow::Result<()> {
        match self {
            Self::Pcf8574Input { device, inputs } => {
                for var_name in inputs.values() {
                    gv.inputs
                        .entry(var_name.clone())
                        .or_insert(VarValue::Bool(false));
                }
                device.init(bus)
            }
            Self::Pcf8574Output { device, outputs } => {
                for var_name in outputs.values() {
                    gv.outputs
                        .entry(var_name.clone())
                        .or_insert(VarValue::Bool(false));
                }
                device.init(bus)
            }
            Self::Max7311Output { device, outputs } => {
                for var_name in outputs.values() {
                    gv.outputs
                        .entry(var_name.clone())
                        .or_insert(VarValue::Bool(false));
                }
                device.init(bus)
            }
        }
    }

    fn before_cycle(&mut self, bus: &mut dyn I2cBus, gv: &mut Gv) -> anyhow::Result<()> {
        if let Self::Pcf8574Input { device, inputs } = self {
            device.read(bus)?;
            for (pin, var_name) in inputs {
                gv.inputs
                    .insert(var_name.clone(), VarValue::Bool(device.get_input(*pin)));
            }
        }

        Ok(())
    }

    fn after_cycle(&mut self, bus: &mut dyn I2cBus, gv: &Gv) -> anyhow::Result<()> {
        match self {
            Self::Pcf8574Input { .. } => {}
            Self::Pcf8574Output { device, outputs } => {
                apply_outputs(device, outputs, gv)?;
                device.write(bus)?;
            }
            Self::Max7311Output { device, outputs } => {
                apply_outputs(device, outputs, gv)?;
                device.write(bus)?;
            }
        }

        Ok(())
    }
}

trait OutputDevice {
    fn set_output(&mut self, pos: u8, value: bool);
}

impl OutputDevice for Pcf8574Output {
    fn set_output(&mut self, pos: u8, value: bool) {
        Pcf8574Output::set_output(self, pos, value);
    }
}

impl OutputDevice for Max7311Output {
    fn set_output(&mut self, pos: u8, value: bool) {
        Max7311Output::set_output(self, pos, value);
    }
}

fn apply_outputs(
    device: &mut dyn OutputDevice,
    outputs: &BTreeMap<u8, String>,
    gv: &Gv,
) -> anyhow::Result<()> {
    for (pin, var_name) in outputs {
        let value = match gv.outputs.get(var_name) {
            Some(VarValue::Bool(value)) => *value,
            Some(other) => {
                anyhow::bail!("I2C output variable '{var_name}' must be bool, got {other:?}")
            }
            None => anyhow::bail!("I2C output variable '{var_name}' is missing"),
        };
        device.set_output(*pin, value);
    }

    Ok(())
}

pub struct I2cIo<B: I2cBus> {
    bus: B,
    components: Vec<I2cComponent>,
}

impl<B: I2cBus> I2cIo<B> {
    pub fn new(bus: B, components: Vec<I2cComponent>) -> Self {
        Self { bus, components }
    }

    pub fn bus(&self) -> &B {
        &self.bus
    }

    pub fn bus_mut(&mut self) -> &mut B {
        &mut self.bus
    }
}

impl<B: I2cBus> TaskIo for I2cIo<B> {
    fn init(&mut self, gv: &mut Gv) -> anyhow::Result<()> {
        self.bus.open()?;
        for component in &mut self.components {
            component.init(&mut self.bus, gv)?;
        }
        Ok(())
    }

    fn before_cycle(&mut self, gv: &mut Gv) -> anyhow::Result<()> {
        for component in &mut self.components {
            component.before_cycle(&mut self.bus, gv)?;
        }
        Ok(())
    }

    fn after_cycle(&mut self, gv: &mut Gv) -> anyhow::Result<()> {
        for component in &mut self.components {
            component.after_cycle(&mut self.bus, gv)?;
        }
        Ok(())
    }
}

impl<B: I2cBus> Drop for I2cIo<B> {
    fn drop(&mut self) {
        let _ = self.bus.close();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[derive(Debug, Clone, PartialEq, Eq)]
    enum Call {
        Open,
        Read { address: u8, len: usize },
        Write { address: u8, data: Vec<u8> },
        Close,
    }

    #[derive(Debug, Default)]
    struct FakeI2cBus {
        calls: Vec<Call>,
        reads: Vec<u8>,
    }

    impl FakeI2cBus {
        fn with_reads(reads: impl Into<Vec<u8>>) -> Self {
            Self {
                calls: Vec::new(),
                reads: reads.into(),
            }
        }
    }

    impl I2cBus for FakeI2cBus {
        fn open(&mut self) -> anyhow::Result<()> {
            self.calls.push(Call::Open);
            Ok(())
        }

        fn read(&mut self, address: u8, data: &mut [u8]) -> anyhow::Result<()> {
            self.calls.push(Call::Read {
                address,
                len: data.len(),
            });
            for byte in data {
                *byte = self.reads.remove(0);
            }
            Ok(())
        }

        fn write(&mut self, address: u8, data: &[u8]) -> anyhow::Result<()> {
            self.calls.push(Call::Write {
                address,
                data: data.to_vec(),
            });
            Ok(())
        }

        fn close(&mut self) -> anyhow::Result<()> {
            self.calls.push(Call::Close);
            Ok(())
        }
    }

    #[test]
    fn pcf8574_output_reads_initial_state_inverts_bits_and_writes_only_changes() {
        let mut output = Pcf8574Output::new(0x20);
        let mut bus = FakeI2cBus::with_reads([0x55]);

        output.init(&mut bus).expect("init should read state");
        output.set_output(0, true);
        output.write(&mut bus).expect("write should run");
        output
            .write(&mut bus)
            .expect("unchanged write should be skipped");

        assert_eq!(
            bus.calls,
            vec![
                Call::Read {
                    address: 0x20,
                    len: 1
                },
                Call::Write {
                    address: 0x20,
                    data: vec![0x54]
                }
            ]
        );
    }

    #[test]
    fn pcf8574_input_reads_and_inverts_bits() {
        let mut input = Pcf8574Input::new(0x3b);
        let mut bus = FakeI2cBus::with_reads([0b1111_1101]);

        input.read(&mut bus).expect("read should run");

        assert!(!input.get_input(0));
        assert!(input.get_input(1));
        assert_eq!(
            bus.calls,
            vec![Call::Read {
                address: 0x3b,
                len: 1
            }]
        );
    }

    #[test]
    fn max7311_output_initializes_port_and_writes_only_changes() {
        let mut output = Max7311Output::new(0x20);
        let mut bus = FakeI2cBus::with_reads([0xff]);

        output.init(&mut bus).expect("init should run");
        output.set_output(1, true);
        output.write(&mut bus).expect("write should run");
        output
            .write(&mut bus)
            .expect("unchanged write should be skipped");

        assert_eq!(
            bus.calls,
            vec![
                Call::Write {
                    address: 0x20,
                    data: vec![0x06, 0x00]
                },
                Call::Write {
                    address: 0x20,
                    data: vec![0x02]
                },
                Call::Read {
                    address: 0x20,
                    len: 1
                },
                Call::Write {
                    address: 0x20,
                    data: vec![0x02, 0b1111_1101]
                },
            ]
        );
    }

    #[test]
    fn i2c_io_creates_global_vars_copies_inputs_and_outputs() {
        let bus = FakeI2cBus::with_reads([0xff, 0xff, 0b1111_1101]);
        let components = vec![
            I2cComponent::Pcf8574Input {
                device: Pcf8574Input::new(0x3b),
                inputs: BTreeMap::from([(1, "button".to_string())]),
            },
            I2cComponent::Pcf8574Output {
                device: Pcf8574Output::new(0x20),
                outputs: BTreeMap::from([(2, "light".to_string())]),
            },
        ];
        let mut io = I2cIo::new(bus, components);
        let mut gv = Gv::default();

        io.init(&mut gv).expect("init should run");
        assert_eq!(gv.inputs["button"], VarValue::Bool(false));
        assert_eq!(gv.outputs["light"], VarValue::Bool(false));

        io.before_cycle(&mut gv).expect("before cycle should run");
        assert_eq!(gv.inputs["button"], VarValue::Bool(true));

        gv.outputs.insert("light".to_string(), VarValue::Bool(true));
        io.after_cycle(&mut gv).expect("after cycle should run");

        assert_eq!(
            io.bus().calls,
            vec![
                Call::Open,
                Call::Read {
                    address: 0x3b,
                    len: 1
                },
                Call::Read {
                    address: 0x20,
                    len: 1
                },
                Call::Read {
                    address: 0x3b,
                    len: 1
                },
                Call::Write {
                    address: 0x20,
                    data: vec![0b1111_1011]
                },
            ]
        );
    }

    #[test]
    fn parses_i2c_toml_config() {
        let config: I2cIoConfig = toml::from_str(
            r#"
bus = "/dev/i2c-1"

[components."0x3b"]
type = "pcf8574"
direction = "input"
inputs = { 0 = "button_up", 1 = "button_down" }

[components."0x20"]
type = "max7311"
direction = "output"
outputs = { 0 = "blind_up", 1 = "blind_down" }
"#,
        )
        .expect("I2C config should parse");

        assert_eq!(config.bus, "/dev/i2c-1");
        assert_eq!(config.components["0x3b"].inputs["0"], "button_up");
        assert_eq!(config.components["0x20"].outputs["1"], "blind_down");
    }

    #[test]
    fn rejects_invalid_i2c_pin_numbers() {
        let config: I2cIoConfig = toml::from_str(
            r#"
bus = "/dev/i2c-1"

[components."0x3b"]
type = "pcf8574"
direction = "input"
inputs = { 8 = "button_up" }
"#,
        )
        .expect("I2C config should parse");

        assert!(matches!(
            config.into_io(),
            Err(I2cConfigError::InvalidPin(pin)) if pin == "8"
        ));
    }
}
