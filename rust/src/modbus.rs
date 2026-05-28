use std::{collections::BTreeMap, time::Duration};

use serde::Deserialize;
use thiserror::Error;
use tokio_modbus::{
    client::sync::{rtu, Context as ModbusContext, Reader, Writer},
    slave::SlaveContext,
    Slave,
};
use tokio_serial::{DataBits, Parity, StopBits};

use crate::{
    gv::{Gv, VarValue},
    runtime::TaskIo,
};

pub trait ModbusClient {
    fn connect(&mut self) -> anyhow::Result<()>;
    fn read_input_bits(
        &mut self,
        slave: u8,
        address: u16,
        values: &mut [bool],
    ) -> anyhow::Result<()>;
    fn write_bits(&mut self, slave: u8, address: u16, values: &[bool]) -> anyhow::Result<()>;
    fn close(&mut self) -> anyhow::Result<()>;
}

pub struct TokioModbusRtuClient {
    path: String,
    baud: u32,
    data_bit: u8,
    parity: ModbusParity,
    stop_bit: u8,
    timeout_millis: u64,
    context: Option<ModbusContext>,
}

impl TokioModbusRtuClient {
    pub fn new(
        path: String,
        baud: u32,
        data_bit: u8,
        parity: ModbusParity,
        stop_bit: u8,
        timeout_millis: u64,
    ) -> Self {
        Self {
            path,
            baud,
            data_bit,
            parity,
            stop_bit,
            timeout_millis,
            context: None,
        }
    }

    fn context(&mut self) -> anyhow::Result<&mut ModbusContext> {
        self.context
            .as_mut()
            .ok_or_else(|| anyhow::anyhow!("Modbus RTU client is not connected"))
    }
}

impl ModbusClient for TokioModbusRtuClient {
    fn connect(&mut self) -> anyhow::Result<()> {
        let builder = tokio_serial::new(&self.path, self.baud)
            .data_bits(to_tokio_data_bits(self.data_bit)?)
            .parity(to_tokio_parity(self.parity))
            .stop_bits(to_tokio_stop_bits(self.stop_bit)?);

        let mut context = rtu::connect(&builder)?;
        context.set_timeout(Duration::from_millis(self.timeout_millis));
        self.context = Some(context);
        Ok(())
    }

    fn read_input_bits(
        &mut self,
        slave: u8,
        address: u16,
        values: &mut [bool],
    ) -> anyhow::Result<()> {
        let quantity = u16::try_from(values.len())
            .map_err(|_| anyhow::anyhow!("Modbus read quantity exceeds u16"))?;
        let context = self.context()?;
        context.set_slave(Slave(slave));
        let read_values = flatten_modbus_result(context.read_discrete_inputs(address, quantity))?;
        if read_values.len() != values.len() {
            anyhow::bail!(
                "Modbus read returned {} value(s), expected {}",
                read_values.len(),
                values.len()
            );
        }
        values.copy_from_slice(&read_values);
        Ok(())
    }

    fn write_bits(&mut self, slave: u8, address: u16, values: &[bool]) -> anyhow::Result<()> {
        let context = self.context()?;
        context.set_slave(Slave(slave));
        flatten_modbus_result(context.write_multiple_coils(address, values))
    }

    fn close(&mut self) -> anyhow::Result<()> {
        self.context.take();
        Ok(())
    }
}

fn flatten_modbus_result<T>(result: tokio_modbus::Result<T>) -> anyhow::Result<T> {
    result
        .map_err(|err| anyhow::anyhow!("Modbus transport error: {err}"))?
        .map_err(|exception| anyhow::anyhow!("Modbus exception response: {exception:?}"))
}

fn to_tokio_data_bits(data_bit: u8) -> Result<DataBits, ModbusConfigError> {
    match data_bit {
        5 => Ok(DataBits::Five),
        6 => Ok(DataBits::Six),
        7 => Ok(DataBits::Seven),
        8 => Ok(DataBits::Eight),
        _ => Err(ModbusConfigError::InvalidDataBits(data_bit)),
    }
}

fn to_tokio_parity(parity: ModbusParity) -> Parity {
    match parity {
        ModbusParity::N => Parity::None,
        ModbusParity::E => Parity::Even,
        ModbusParity::O => Parity::Odd,
    }
}

fn to_tokio_stop_bits(stop_bit: u8) -> Result<StopBits, ModbusConfigError> {
    match stop_bit {
        1 => Ok(StopBits::One),
        2 => Ok(StopBits::Two),
        _ => Err(ModbusConfigError::InvalidStopBits(stop_bit)),
    }
}

#[derive(Debug, Deserialize)]
pub struct ModbusRtuIoConfig {
    pub path: String,
    pub baud: u32,
    pub data_bit: u8,
    pub parity: ModbusParity,
    pub stop_bit: u8,
    #[serde(default = "default_modbus_timeout_millis")]
    pub timeout_millis: u64,
    #[serde(default)]
    pub components: Vec<ModbusComponentConfig>,
}

const fn default_modbus_timeout_millis() -> u64 {
    500
}

impl ModbusRtuIoConfig {
    pub fn into_io(self) -> Result<ModbusIo<TokioModbusRtuClient>, ModbusConfigError> {
        self.validate_serial_config()?;
        let mut components = Vec::with_capacity(self.components.len());
        for component in self.components {
            components.push(component.into_component()?);
        }

        Ok(ModbusIo::new(
            TokioModbusRtuClient::new(
                self.path,
                self.baud,
                self.data_bit,
                self.parity,
                self.stop_bit,
                self.timeout_millis,
            ),
            components,
        ))
    }

    fn validate_serial_config(&self) -> Result<(), ModbusConfigError> {
        to_tokio_data_bits(self.data_bit)?;
        to_tokio_stop_bits(self.stop_bit)?;
        if self.timeout_millis == 0 {
            return Err(ModbusConfigError::InvalidTimeoutMillis(self.timeout_millis));
        }
        Ok(())
    }
}

#[derive(Debug, Clone, Copy, Deserialize, PartialEq, Eq)]
pub enum ModbusParity {
    N,
    E,
    O,
}

#[derive(Debug, Deserialize)]
pub struct ModbusComponentConfig {
    #[serde(rename = "type")]
    pub component_type: String,
    pub slave: u8,
    #[serde(default)]
    pub inputs: BTreeMap<String, String>,
    #[serde(default)]
    pub outputs: BTreeMap<String, String>,
}

impl ModbusComponentConfig {
    fn into_component(self) -> Result<ModbusComponent, ModbusConfigError> {
        match self.component_type.as_str() {
            "WP8026ADAM" => Ok(ModbusComponent::Wp8026Adam {
                device: Wp8026Adam::new(self.slave),
                inputs: parse_pin_map(self.inputs)?,
            }),
            "R4S8CRMB" => Ok(ModbusComponent::R4s8Crmb {
                device: R4s8Crmb::new(self.slave),
                outputs: parse_pin_map(self.outputs)?,
            }),
            _ => Err(ModbusConfigError::UnknownComponentType(self.component_type)),
        }
    }
}

#[derive(Debug, Error, PartialEq, Eq)]
pub enum ModbusConfigError {
    #[error("invalid Modbus component pin '{0}'")]
    InvalidPin(String),
    #[error("unknown Modbus component type '{0}'")]
    UnknownComponentType(String),
    #[error("invalid Modbus data bits setting '{0}'")]
    InvalidDataBits(u8),
    #[error("invalid Modbus stop bits setting '{0}'")]
    InvalidStopBits(u8),
    #[error("invalid Modbus timeout_millis setting '{0}'")]
    InvalidTimeoutMillis(u64),
}

fn parse_pin(pin: &str) -> Result<u8, ModbusConfigError> {
    let pin = pin
        .parse::<u8>()
        .map_err(|_| ModbusConfigError::InvalidPin(pin.to_string()))?;
    if pin > 7 {
        return Err(ModbusConfigError::InvalidPin(pin.to_string()));
    }
    Ok(pin)
}

fn parse_pin_map(
    pin_map: BTreeMap<String, String>,
) -> Result<BTreeMap<u8, String>, ModbusConfigError> {
    pin_map
        .into_iter()
        .map(|(pin, var_name)| Ok((parse_pin(&pin)?, var_name)))
        .collect()
}

pub struct Wp8026Adam {
    slave: u8,
    inputs: [bool; 8],
}

impl Wp8026Adam {
    const INPUT_ADDRESS: u16 = 0x0008;

    pub fn new(slave: u8) -> Self {
        Self {
            slave,
            inputs: [false; 8],
        }
    }

    pub fn init(&mut self, client: &mut dyn ModbusClient) -> anyhow::Result<()> {
        self.read(client)
    }

    pub fn read(&mut self, client: &mut dyn ModbusClient) -> anyhow::Result<()> {
        client.read_input_bits(self.slave, Self::INPUT_ADDRESS, &mut self.inputs)
    }

    pub fn get_input(&self, pos: u8) -> anyhow::Result<bool> {
        self.inputs
            .get(usize::from(pos))
            .copied()
            .ok_or_else(|| anyhow::anyhow!("invalid position {pos} for WP8026ADAM"))
    }
}

pub struct R4s8Crmb {
    slave: u8,
    outputs: [bool; 8],
    last_outputs: [bool; 8],
}

impl R4s8Crmb {
    const OUTPUT_ADDRESS: u16 = 0x0000;

    pub fn new(slave: u8) -> Self {
        Self {
            slave,
            outputs: [false; 8],
            last_outputs: [false; 8],
        }
    }

    pub fn init(&mut self, client: &mut dyn ModbusClient) -> anyhow::Result<()> {
        self.write(client)
    }

    pub fn set_output(&mut self, pos: u8, value: bool) -> anyhow::Result<()> {
        let output = self
            .outputs
            .get_mut(usize::from(pos))
            .ok_or_else(|| anyhow::anyhow!("invalid position {pos} for R4S8CRMB"))?;
        *output = value;
        Ok(())
    }

    pub fn write(&mut self, client: &mut dyn ModbusClient) -> anyhow::Result<()> {
        if self.outputs == self.last_outputs {
            return Ok(());
        }

        client.write_bits(self.slave, Self::OUTPUT_ADDRESS, &self.outputs)?;
        self.last_outputs = self.outputs;
        Ok(())
    }
}

pub enum ModbusComponent {
    Wp8026Adam {
        device: Wp8026Adam,
        inputs: BTreeMap<u8, String>,
    },
    R4s8Crmb {
        device: R4s8Crmb,
        outputs: BTreeMap<u8, String>,
    },
}

impl ModbusComponent {
    fn init(&mut self, client: &mut dyn ModbusClient, gv: &mut Gv) -> anyhow::Result<()> {
        match self {
            Self::Wp8026Adam { device, inputs } => {
                for var_name in inputs.values() {
                    gv.inputs
                        .entry(var_name.clone())
                        .or_insert(VarValue::Bool(false));
                }
                device.init(client)
            }
            Self::R4s8Crmb { device, outputs } => {
                for var_name in outputs.values() {
                    gv.outputs
                        .entry(var_name.clone())
                        .or_insert(VarValue::Bool(false));
                }
                device.init(client)
            }
        }
    }

    fn before_cycle(&mut self, client: &mut dyn ModbusClient, gv: &mut Gv) -> anyhow::Result<()> {
        if let Self::Wp8026Adam { device, inputs } = self {
            device.read(client)?;
            for (pin, var_name) in inputs {
                gv.inputs
                    .insert(var_name.clone(), VarValue::Bool(device.get_input(*pin)?));
            }
        }

        Ok(())
    }

    fn after_cycle(&mut self, client: &mut dyn ModbusClient, gv: &Gv) -> anyhow::Result<()> {
        match self {
            Self::Wp8026Adam { .. } => {}
            Self::R4s8Crmb { device, outputs } => {
                for (pin, var_name) in outputs {
                    let value = match gv.outputs.get(var_name) {
                        Some(VarValue::Bool(value)) => *value,
                        Some(other) => anyhow::bail!(
                            "Modbus output variable '{var_name}' must be bool, got {other:?}"
                        ),
                        None => anyhow::bail!("Modbus output variable '{var_name}' is missing"),
                    };
                    device.set_output(*pin, value)?;
                }
                device.write(client)?;
            }
        }

        Ok(())
    }
}

pub struct ModbusIo<C: ModbusClient> {
    client: C,
    components: Vec<ModbusComponent>,
}

impl<C: ModbusClient> ModbusIo<C> {
    pub fn new(client: C, components: Vec<ModbusComponent>) -> Self {
        Self { client, components }
    }

    pub fn client(&self) -> &C {
        &self.client
    }
}

impl<C: ModbusClient> TaskIo for ModbusIo<C> {
    fn init(&mut self, gv: &mut Gv) -> anyhow::Result<()> {
        self.client.connect()?;
        for component in &mut self.components {
            component.init(&mut self.client, gv)?;
        }
        Ok(())
    }

    fn before_cycle(&mut self, gv: &mut Gv) -> anyhow::Result<()> {
        for component in &mut self.components {
            component.before_cycle(&mut self.client, gv)?;
        }
        Ok(())
    }

    fn after_cycle(&mut self, gv: &mut Gv) -> anyhow::Result<()> {
        for component in &mut self.components {
            component.after_cycle(&mut self.client, gv)?;
        }
        Ok(())
    }
}

impl<C: ModbusClient> Drop for ModbusIo<C> {
    fn drop(&mut self) {
        let _ = self.client.close();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[derive(Debug, Clone, PartialEq, Eq)]
    enum Call {
        Connect,
        ReadInputBits {
            slave: u8,
            address: u16,
            quantity: usize,
        },
        WriteBits {
            slave: u8,
            address: u16,
            values: [bool; 8],
        },
        Close,
    }

    #[derive(Debug, Default)]
    struct FakeModbusClient {
        calls: Vec<Call>,
        input_reads: Vec<[bool; 8]>,
    }

    impl FakeModbusClient {
        fn with_input_reads(input_reads: Vec<[bool; 8]>) -> Self {
            Self {
                calls: Vec::new(),
                input_reads,
            }
        }
    }

    impl ModbusClient for FakeModbusClient {
        fn connect(&mut self) -> anyhow::Result<()> {
            self.calls.push(Call::Connect);
            Ok(())
        }

        fn read_input_bits(
            &mut self,
            slave: u8,
            address: u16,
            values: &mut [bool],
        ) -> anyhow::Result<()> {
            self.calls.push(Call::ReadInputBits {
                slave,
                address,
                quantity: values.len(),
            });
            let next_read = self.input_reads.remove(0);
            values.copy_from_slice(&next_read[..values.len()]);
            Ok(())
        }

        fn write_bits(&mut self, slave: u8, address: u16, values: &[bool]) -> anyhow::Result<()> {
            self.calls.push(Call::WriteBits {
                slave,
                address,
                values: values.try_into().expect("R4S8CRMB writes 8 outputs"),
            });
            Ok(())
        }

        fn close(&mut self) -> anyhow::Result<()> {
            self.calls.push(Call::Close);
            Ok(())
        }
    }

    #[test]
    fn wp8026adam_reads_upper_8_input_bits_from_reference_address() {
        let mut input = Wp8026Adam::new(1);
        let mut client = FakeModbusClient::with_input_reads(vec![[
            false, true, false, false, false, false, false, false,
        ]]);

        input.read(&mut client).expect("read should run");

        assert!(!input.get_input(0).expect("input should exist"));
        assert!(input.get_input(1).expect("input should exist"));
        assert_eq!(
            client.calls,
            vec![Call::ReadInputBits {
                slave: 1,
                address: 0x0008,
                quantity: 8
            }]
        );
    }

    #[test]
    fn r4s8crmb_writes_outputs_to_reference_address_only_when_changed() {
        let mut output = R4s8Crmb::new(2);
        let mut client = FakeModbusClient::default();

        output
            .init(&mut client)
            .expect("unchanged init write is skipped");
        output.set_output(1, true).expect("pin should exist");
        output.write(&mut client).expect("write should run");
        output
            .write(&mut client)
            .expect("unchanged write should be skipped");

        assert_eq!(
            client.calls,
            vec![Call::WriteBits {
                slave: 2,
                address: 0x0000,
                values: [false, true, false, false, false, false, false, false]
            }]
        );
    }

    #[test]
    fn modbus_io_creates_global_vars_copies_inputs_and_outputs() {
        let client = FakeModbusClient::with_input_reads(vec![
            [false, false, false, false, false, false, false, false],
            [false, true, false, false, false, false, false, false],
        ]);
        let components = vec![
            ModbusComponent::Wp8026Adam {
                device: Wp8026Adam::new(1),
                inputs: BTreeMap::from([(1, "button".to_string())]),
            },
            ModbusComponent::R4s8Crmb {
                device: R4s8Crmb::new(1),
                outputs: BTreeMap::from([(2, "light".to_string())]),
            },
        ];
        let mut io = ModbusIo::new(client, components);
        let mut gv = Gv::default();

        io.init(&mut gv).expect("init should run");
        assert_eq!(gv.inputs["button"], VarValue::Bool(false));
        assert_eq!(gv.outputs["light"], VarValue::Bool(false));

        io.before_cycle(&mut gv).expect("before cycle should run");
        assert_eq!(gv.inputs["button"], VarValue::Bool(true));

        gv.outputs.insert("light".to_string(), VarValue::Bool(true));
        io.after_cycle(&mut gv).expect("after cycle should run");

        assert_eq!(
            io.client().calls,
            vec![
                Call::Connect,
                Call::ReadInputBits {
                    slave: 1,
                    address: 0x0008,
                    quantity: 8
                },
                Call::ReadInputBits {
                    slave: 1,
                    address: 0x0008,
                    quantity: 8
                },
                Call::WriteBits {
                    slave: 1,
                    address: 0x0000,
                    values: [false, false, true, false, false, false, false, false]
                },
            ]
        );
    }

    #[test]
    fn parses_modbus_toml_config() {
        let config: ModbusRtuIoConfig = toml::from_str(
            r#"
path = "/dev/ttyUSB0"
baud = 9600
data_bit = 8
parity = "N"
stop_bit = 1

[[components]]
type = "WP8026ADAM"
slave = 1
inputs = { 0 = "one", 1 = "two" }

[[components]]
type = "R4S8CRMB"
slave = 1
outputs = { 0 = "one", 1 = "two" }
"#,
        )
        .expect("Modbus config should parse");

        assert_eq!(config.path, "/dev/ttyUSB0");
        assert_eq!(config.baud, 9600);
        assert_eq!(config.timeout_millis, 500);
        assert_eq!(config.components[0].inputs["0"], "one");
        assert_eq!(config.components[1].outputs["1"], "two");
    }

    #[test]
    fn maps_supported_serial_settings_to_tokio_serial() {
        assert_eq!(
            to_tokio_data_bits(5).expect("valid data bits"),
            DataBits::Five
        );
        assert_eq!(
            to_tokio_data_bits(6).expect("valid data bits"),
            DataBits::Six
        );
        assert_eq!(
            to_tokio_data_bits(7).expect("valid data bits"),
            DataBits::Seven
        );
        assert_eq!(
            to_tokio_data_bits(8).expect("valid data bits"),
            DataBits::Eight
        );
        assert_eq!(to_tokio_parity(ModbusParity::N), Parity::None);
        assert_eq!(to_tokio_parity(ModbusParity::E), Parity::Even);
        assert_eq!(to_tokio_parity(ModbusParity::O), Parity::Odd);
        assert_eq!(
            to_tokio_stop_bits(1).expect("valid stop bits"),
            StopBits::One
        );
        assert_eq!(
            to_tokio_stop_bits(2).expect("valid stop bits"),
            StopBits::Two
        );
    }

    #[test]
    fn rejects_unsupported_serial_settings() {
        let invalid_data_bits = ModbusRtuIoConfig {
            path: "/dev/ttyUSB0".to_string(),
            baud: 9600,
            data_bit: 9,
            parity: ModbusParity::N,
            stop_bit: 1,
            timeout_millis: 500,
            components: Vec::new(),
        };
        assert!(matches!(
            invalid_data_bits.into_io(),
            Err(ModbusConfigError::InvalidDataBits(9))
        ));

        let invalid_stop_bits = ModbusRtuIoConfig {
            path: "/dev/ttyUSB0".to_string(),
            baud: 9600,
            data_bit: 8,
            parity: ModbusParity::N,
            stop_bit: 3,
            timeout_millis: 500,
            components: Vec::new(),
        };
        assert!(matches!(
            invalid_stop_bits.into_io(),
            Err(ModbusConfigError::InvalidStopBits(3))
        ));

        let invalid_timeout = ModbusRtuIoConfig {
            path: "/dev/ttyUSB0".to_string(),
            baud: 9600,
            data_bit: 8,
            parity: ModbusParity::N,
            stop_bit: 1,
            timeout_millis: 0,
            components: Vec::new(),
        };
        assert!(matches!(
            invalid_timeout.into_io(),
            Err(ModbusConfigError::InvalidTimeoutMillis(0))
        ));
    }

    #[test]
    fn rejects_invalid_modbus_pin_numbers() {
        let config: ModbusRtuIoConfig = toml::from_str(
            r#"
path = "/dev/ttyUSB0"
baud = 9600
data_bit = 8
parity = "N"
stop_bit = 1

[[components]]
type = "WP8026ADAM"
slave = 1
inputs = { 8 = "one" }
"#,
        )
        .expect("Modbus config should parse");

        assert!(matches!(
            config.into_io(),
            Err(ModbusConfigError::InvalidPin(pin)) if pin == "8"
        ));
    }

    #[test]
    fn rejects_unknown_modbus_component_types() {
        let config: ModbusRtuIoConfig = toml::from_str(
            r#"
path = "/dev/ttyUSB0"
baud = 9600
data_bit = 8
parity = "N"
stop_bit = 1

[[components]]
type = "Unknown"
slave = 1
"#,
        )
        .expect("Modbus config should parse");

        assert!(matches!(
            config.into_io(),
            Err(ModbusConfigError::UnknownComponentType(component)) if component == "Unknown"
        ));
    }
}
