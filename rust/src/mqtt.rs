use std::{
    collections::BTreeMap,
    sync::{
        atomic::{AtomicBool, Ordering},
        mpsc::{self, Receiver},
        Arc,
    },
    thread::{self, JoinHandle},
    time::Duration,
};

use rumqttc::{Client, Event, MqttOptions, Packet, QoS};
use serde::Deserialize;
use thiserror::Error;
use tracing::warn;

use crate::{
    gv::{Gv, VarValue},
    runtime::TaskIo,
};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MqttPayloadCodec {
    TextBool,
    BinaryBool,
}

impl Default for MqttPayloadCodec {
    fn default() -> Self {
        Self::TextBool
    }
}

#[derive(Debug, Error, PartialEq, Eq)]
pub enum MqttPayloadCodecError {
    #[error("unknown MQTT payload codec '{0}'")]
    Unknown(String),
    #[error("invalid MQTT boolean payload for codec {codec:?}: {payload:?}")]
    InvalidBoolPayload {
        codec: MqttPayloadCodec,
        payload: Vec<u8>,
    },
}

impl MqttPayloadCodec {
    pub fn parse(value: &str) -> Result<Self, MqttPayloadCodecError> {
        match value {
            "text-bool" => Ok(Self::TextBool),
            "binary-bool" => Ok(Self::BinaryBool),
            other => Err(MqttPayloadCodecError::Unknown(other.to_string())),
        }
    }

    pub fn decode_bool(&self, payload: &[u8]) -> Result<bool, MqttPayloadCodecError> {
        match self {
            Self::TextBool => match std::str::from_utf8(payload).map(str::trim) {
                Ok("1") => Ok(true),
                Ok("0") => Ok(false),
                _ => Err(MqttPayloadCodecError::InvalidBoolPayload {
                    codec: *self,
                    payload: payload.to_vec(),
                }),
            },
            Self::BinaryBool => match payload {
                [0x01] => Ok(true),
                [0x00] => Ok(false),
                _ => Err(MqttPayloadCodecError::InvalidBoolPayload {
                    codec: *self,
                    payload: payload.to_vec(),
                }),
            },
        }
    }

    pub fn encode_true(&self) -> Vec<u8> {
        match self {
            Self::TextBool => b"1".to_vec(),
            Self::BinaryBool => vec![0x01],
        }
    }
}

impl<'de> Deserialize<'de> for MqttPayloadCodec {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let value = String::deserialize(deserializer)?;
        Self::parse(&value).map_err(serde::de::Error::custom)
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MqttMessage {
    pub topic: String,
    pub payload: Vec<u8>,
}

impl MqttMessage {
    pub fn new(topic: impl Into<String>, payload: impl Into<Vec<u8>>) -> Self {
        Self {
            topic: topic.into(),
            payload: payload.into(),
        }
    }
}

pub trait MqttClient {
    fn connect(&mut self) -> anyhow::Result<()>;
    fn subscribe(&mut self, topic: &str) -> anyhow::Result<()>;
    fn drain_received(&mut self) -> anyhow::Result<Vec<MqttMessage>>;
    fn publish(&mut self, topic: &str, payload: &[u8]) -> anyhow::Result<()>;
}

#[derive(Debug, Deserialize)]
pub struct MqttIoConfig {
    #[serde(default)]
    pub payload_codec: MqttPayloadCodec,
    pub client: RumqttcClientConfig,
    #[serde(default)]
    pub inputs: BTreeMap<String, String>,
    #[serde(default)]
    pub outputs: BTreeMap<String, String>,
}

impl MqttIoConfig {
    pub fn into_io(self) -> Result<MqttIo<RumqttcClient>, MqttConfigError> {
        let client = RumqttcClient::new(self.client)?;
        Ok(MqttIo::new(
            client,
            self.inputs,
            self.outputs,
            self.payload_codec,
        ))
    }
}

#[derive(Debug, Deserialize)]
pub struct RumqttcClientConfig {
    pub client_id: String,
    pub address: Option<String>,
    pub host: Option<String>,
    pub username: Option<String>,
    pub password: Option<String>,
    #[serde(default = "default_mqtt_port")]
    pub port: u16,
    #[serde(default = "default_keep_alive_secs")]
    pub keep_alive_secs: u64,
    #[serde(default = "default_request_channel_capacity")]
    pub request_channel_capacity: usize,
}

#[derive(Debug, Error, PartialEq, Eq)]
pub enum MqttConfigError {
    #[error("MQTT client config must define either address or host")]
    MissingAddress,
    #[error("MQTT address '{0}' uses unsupported scheme; only tcp:// and mqtt:// are supported")]
    UnsupportedAddressScheme(String),
    #[error("MQTT address '{0}' must include a host")]
    MissingAddressHost(String),
    #[error("MQTT address '{0}' has an invalid port")]
    InvalidAddressPort(String),
    #[error("MQTT client credentials must define both username and password")]
    IncompleteCredentials,
}

fn default_mqtt_port() -> u16 {
    1883
}

fn default_keep_alive_secs() -> u64 {
    5
}

fn default_request_channel_capacity() -> usize {
    10
}

impl RumqttcClientConfig {
    fn broker(&self) -> Result<(String, u16), MqttConfigError> {
        if let Some(address) = &self.address {
            return parse_broker_address(address, self.port);
        }

        let Some(host) = &self.host else {
            return Err(MqttConfigError::MissingAddress);
        };

        if host.is_empty() {
            return Err(MqttConfigError::MissingAddressHost(host.clone()));
        }

        Ok((host.clone(), self.port))
    }
}

fn parse_broker_address(
    address: &str,
    default_port: u16,
) -> Result<(String, u16), MqttConfigError> {
    let without_scheme = address
        .strip_prefix("tcp://")
        .or_else(|| address.strip_prefix("mqtt://"))
        .ok_or_else(|| MqttConfigError::UnsupportedAddressScheme(address.to_string()))?;

    let (host, port) = match without_scheme.rsplit_once(':') {
        Some((host, port)) => {
            let port = port
                .parse()
                .map_err(|_| MqttConfigError::InvalidAddressPort(address.to_string()))?;
            (host, port)
        }
        None => (without_scheme, default_port),
    };

    if host.is_empty() {
        return Err(MqttConfigError::MissingAddressHost(address.to_string()));
    }

    Ok((host.to_string(), port))
}

pub struct RumqttcClient {
    options: MqttOptions,
    request_channel_capacity: usize,
    client: Option<Client>,
    received: Option<Receiver<MqttMessage>>,
    shutdown_requested: Arc<AtomicBool>,
    event_thread: Option<JoinHandle<()>>,
}

impl RumqttcClient {
    pub fn new(config: RumqttcClientConfig) -> Result<Self, MqttConfigError> {
        let (host, port) = config.broker()?;
        let mut options = MqttOptions::new(config.client_id, host, port);
        options.set_keep_alive(Duration::from_secs(config.keep_alive_secs));
        options.set_request_channel_capacity(config.request_channel_capacity);
        match (config.username, config.password) {
            (Some(username), Some(password)) => {
                options.set_credentials(username, password);
            }
            (None, None) => {}
            _ => return Err(MqttConfigError::IncompleteCredentials),
        }

        Ok(Self {
            options,
            request_channel_capacity: config.request_channel_capacity,
            client: None,
            received: None,
            shutdown_requested: Arc::new(AtomicBool::new(false)),
            event_thread: None,
        })
    }
}

impl MqttClient for RumqttcClient {
    fn connect(&mut self) -> anyhow::Result<()> {
        if self.client.is_some() {
            return Ok(());
        }

        let (client, mut connection) =
            Client::new(self.options.clone(), self.request_channel_capacity);
        let (received_tx, received_rx) = mpsc::channel();
        self.shutdown_requested.store(false, Ordering::SeqCst);
        let shutdown_requested = Arc::clone(&self.shutdown_requested);

        let event_thread = thread::spawn(move || {
            for event in connection.iter() {
                match event {
                    Ok(Event::Incoming(Packet::Publish(publish))) => {
                        if received_tx
                            .send(MqttMessage::new(publish.topic, publish.payload.to_vec()))
                            .is_err()
                        {
                            break;
                        }
                    }
                    Ok(_) => {}
                    Err(error) => {
                        if should_log_mqtt_event_error(&shutdown_requested) {
                            warn!(error = %error, "MQTT connection event failed");
                        }
                        break;
                    }
                }
            }
        });

        self.client = Some(client);
        self.received = Some(received_rx);
        self.event_thread = Some(event_thread);
        Ok(())
    }

    fn subscribe(&mut self, topic: &str) -> anyhow::Result<()> {
        let client = self
            .client
            .as_ref()
            .ok_or_else(|| anyhow::anyhow!("MQTT client is not connected"))?;
        Ok(client.subscribe(topic, QoS::AtMostOnce)?)
    }

    fn drain_received(&mut self) -> anyhow::Result<Vec<MqttMessage>> {
        let received = self
            .received
            .as_ref()
            .ok_or_else(|| anyhow::anyhow!("MQTT client is not connected"))?;
        Ok(received.try_iter().collect())
    }

    fn publish(&mut self, topic: &str, payload: &[u8]) -> anyhow::Result<()> {
        let client = self
            .client
            .as_ref()
            .ok_or_else(|| anyhow::anyhow!("MQTT client is not connected"))?;
        Ok(client.publish(topic, QoS::AtMostOnce, false, payload)?)
    }
}

fn should_log_mqtt_event_error(shutdown_requested: &AtomicBool) -> bool {
    !shutdown_requested.load(Ordering::SeqCst)
}

impl Drop for RumqttcClient {
    fn drop(&mut self) {
        self.shutdown_requested.store(true, Ordering::SeqCst);
        if let Some(client) = self.client.take() {
            let _ = client.disconnect();
        }
        self.received.take();
        if let Some(event_thread) = self.event_thread.take() {
            let _ = event_thread.join();
        }
    }
}

pub struct MqttIo<C> {
    client: C,
    inputs_by_topic: BTreeMap<String, String>,
    outputs_by_var: BTreeMap<String, String>,
    previous_outputs: BTreeMap<String, bool>,
    payload_codec: MqttPayloadCodec,
}

impl<C> MqttIo<C> {
    pub fn new(
        client: C,
        inputs_by_topic: BTreeMap<String, String>,
        outputs_by_topic: BTreeMap<String, String>,
        payload_codec: MqttPayloadCodec,
    ) -> Self {
        let outputs_by_var = outputs_by_topic
            .into_iter()
            .map(|(topic, var_name)| (var_name, topic))
            .collect();

        Self {
            client,
            inputs_by_topic,
            outputs_by_var,
            previous_outputs: BTreeMap::new(),
            payload_codec,
        }
    }

    pub fn client(&self) -> &C {
        &self.client
    }

    pub fn client_mut(&mut self) -> &mut C {
        &mut self.client
    }
}

impl<C: MqttClient> TaskIo for MqttIo<C> {
    fn init(&mut self, _gv: &mut Gv) -> anyhow::Result<()> {
        self.client.connect()?;

        for topic in self.inputs_by_topic.keys() {
            self.client.subscribe(topic)?;
        }

        self.previous_outputs = self
            .outputs_by_var
            .keys()
            .map(|var_name| (var_name.clone(), false))
            .collect();

        Ok(())
    }

    fn before_cycle(&mut self, gv: &mut Gv) -> anyhow::Result<()> {
        for var_name in self.inputs_by_topic.values() {
            gv.inputs.insert(var_name.clone(), VarValue::Bool(false));
        }

        for message in self.client.drain_received()? {
            let Some(var_name) = self.inputs_by_topic.get(&message.topic) else {
                continue;
            };

            match self.payload_codec.decode_bool(&message.payload) {
                Ok(value) => {
                    gv.inputs.insert(var_name.clone(), VarValue::Bool(value));
                }
                Err(error) => {
                    warn!(
                        topic = message.topic,
                        error = %error,
                        "ignoring invalid MQTT input payload"
                    );
                }
            }
        }

        Ok(())
    }

    fn after_cycle(&mut self, gv: &mut Gv) -> anyhow::Result<()> {
        for (var_name, topic) in &self.outputs_by_var {
            let current = match gv.outputs.get(var_name) {
                Some(VarValue::Bool(value)) => *value,
                Some(other) => {
                    anyhow::bail!("MQTT output variable '{var_name}' must be bool, got {other:?}")
                }
                None => anyhow::bail!("MQTT output variable '{var_name}' is missing"),
            };

            let previous = self
                .previous_outputs
                .get(var_name)
                .copied()
                .unwrap_or(false);

            if !previous && current {
                self.client
                    .publish(topic, &self.payload_codec.encode_true())?;
            }

            self.previous_outputs.insert(var_name.clone(), current);
        }

        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[derive(Debug, Default)]
    struct FakeMqttClient {
        connected: bool,
        subscribed: Vec<String>,
        received: Vec<MqttMessage>,
        published: Vec<MqttMessage>,
    }

    impl FakeMqttClient {
        fn push_received(&mut self, topic: &str, payload: impl Into<Vec<u8>>) {
            self.received.push(MqttMessage::new(topic, payload));
        }
    }

    impl MqttClient for FakeMqttClient {
        fn connect(&mut self) -> anyhow::Result<()> {
            self.connected = true;
            Ok(())
        }

        fn subscribe(&mut self, topic: &str) -> anyhow::Result<()> {
            self.subscribed.push(topic.to_string());
            Ok(())
        }

        fn drain_received(&mut self) -> anyhow::Result<Vec<MqttMessage>> {
            Ok(std::mem::take(&mut self.received))
        }

        fn publish(&mut self, topic: &str, payload: &[u8]) -> anyhow::Result<()> {
            self.published.push(MqttMessage::new(topic, payload));
            Ok(())
        }
    }

    fn text_io() -> MqttIo<FakeMqttClient> {
        MqttIo::new(
            FakeMqttClient::default(),
            BTreeMap::from([("/input".to_string(), "button".to_string())]),
            BTreeMap::from([("/output".to_string(), "light".to_string())]),
            MqttPayloadCodec::TextBool,
        )
    }

    #[test]
    fn text_bool_codec_decodes_and_encodes_reference_payloads() {
        let codec = MqttPayloadCodec::TextBool;

        assert_eq!(codec.decode_bool(b"1"), Ok(true));
        assert_eq!(codec.decode_bool(b"0"), Ok(false));
        assert_eq!(codec.decode_bool(b" 1\n"), Ok(true));
        assert_eq!(codec.encode_true(), b"1".to_vec());
        assert!(codec.decode_bool(b"true").is_err());
    }

    #[test]
    fn binary_bool_codec_decodes_and_encodes_single_byte_payloads() {
        let codec = MqttPayloadCodec::BinaryBool;

        assert_eq!(codec.decode_bool(&[0x01]), Ok(true));
        assert_eq!(codec.decode_bool(&[0x00]), Ok(false));
        assert_eq!(codec.encode_true(), vec![0x01]);
        assert!(codec.decode_bool(b"1").is_err());
    }

    #[test]
    fn parses_toml_mqtt_io_config() {
        let config: MqttIoConfig = toml::from_str(
            r#"
payload_codec = "binary-bool"

[client]
address = "tcp://mqtt.local:1884"
client_id = "homeautomation-plc-test"
keep_alive_secs = 10
request_channel_capacity = 20

[inputs]
"/button" = "button"

[outputs]
"/light" = "light"
"#,
        )
        .expect("MQTT config should parse");

        assert_eq!(config.payload_codec, MqttPayloadCodec::BinaryBool);
        assert_eq!(config.inputs["/button"], "button");
        assert_eq!(config.outputs["/light"], "light");

        let (host, port) = config.client.broker().expect("broker should parse");
        assert_eq!(host, "mqtt.local");
        assert_eq!(port, 1884);
    }

    #[test]
    fn parses_mqtt_host_port_client_config() {
        let config = RumqttcClientConfig {
            client_id: "test".to_string(),
            address: None,
            host: Some("localhost".to_string()),
            username: None,
            password: None,
            port: 1883,
            keep_alive_secs: 5,
            request_channel_capacity: 10,
        };

        assert_eq!(
            config.broker().expect("broker should parse"),
            ("localhost".to_string(), 1883)
        );
    }

    #[test]
    fn rejects_unsupported_mqtt_address_scheme() {
        assert_eq!(
            parse_broker_address("ssl://localhost:8883", 1883),
            Err(MqttConfigError::UnsupportedAddressScheme(
                "ssl://localhost:8883".to_string()
            ))
        );
    }

    #[test]
    fn rejects_incomplete_mqtt_credentials() {
        let config = RumqttcClientConfig {
            client_id: "test".to_string(),
            address: Some("tcp://localhost:1883".to_string()),
            host: None,
            username: Some("user".to_string()),
            password: None,
            port: 1883,
            keep_alive_secs: 5,
            request_channel_capacity: 10,
        };

        assert!(matches!(
            RumqttcClient::new(config),
            Err(MqttConfigError::IncompleteCredentials)
        ));
    }

    #[test]
    fn mqtt_event_errors_are_suppressed_after_shutdown_is_requested() {
        let shutdown_requested = AtomicBool::new(false);
        assert!(should_log_mqtt_event_error(&shutdown_requested));

        shutdown_requested.store(true, Ordering::SeqCst);
        assert!(!should_log_mqtt_event_error(&shutdown_requested));
    }

    #[test]
    fn uses_default_port_for_mqtt_address_without_port() {
        assert_eq!(
            parse_broker_address("tcp://localhost", 1883).expect("broker should parse"),
            ("localhost".to_string(), 1883)
        );
    }

    #[test]
    fn init_connects_and_subscribes_to_input_topics() {
        let mut io = text_io();
        let mut gv = Gv::default();

        io.init(&mut gv).expect("init should run");

        assert!(io.client().connected);
        assert_eq!(io.client().subscribed, vec!["/input"]);
    }

    #[test]
    fn before_cycle_resets_inputs_and_maps_received_messages() {
        let mut io = text_io();
        let mut gv = Gv::default();
        gv.inputs.insert("button".to_string(), VarValue::Bool(true));
        io.client_mut().push_received("/input", b"0".to_vec());

        io.before_cycle(&mut gv).expect("before cycle should run");

        assert_eq!(gv.inputs["button"], VarValue::Bool(false));

        io.client_mut().push_received("/input", b"1".to_vec());
        io.before_cycle(&mut gv).expect("before cycle should run");

        assert_eq!(gv.inputs["button"], VarValue::Bool(true));
    }

    #[test]
    fn before_cycle_ignores_unknown_topics_and_invalid_payloads() {
        let mut io = text_io();
        let mut gv = Gv::default();
        io.client_mut().push_received("/unknown", b"1".to_vec());
        io.client_mut().push_received("/input", b"invalid".to_vec());

        io.before_cycle(&mut gv).expect("before cycle should run");

        assert_eq!(gv.inputs["button"], VarValue::Bool(false));
    }

    #[test]
    fn after_cycle_publishes_only_rising_edges() {
        let mut io = text_io();
        let mut gv = Gv::default();

        io.init(&mut gv).expect("init should run");
        gv.outputs
            .insert("light".to_string(), VarValue::Bool(false));
        io.after_cycle(&mut gv).expect("after cycle should run");
        assert!(io.client().published.is_empty());

        gv.outputs.insert("light".to_string(), VarValue::Bool(true));
        io.after_cycle(&mut gv).expect("after cycle should run");
        assert_eq!(
            io.client().published,
            vec![MqttMessage::new("/output", b"1".to_vec())]
        );

        io.after_cycle(&mut gv).expect("after cycle should run");
        assert_eq!(io.client().published.len(), 1);

        gv.outputs
            .insert("light".to_string(), VarValue::Bool(false));
        io.after_cycle(&mut gv).expect("after cycle should run");
        assert_eq!(io.client().published.len(), 1);
    }

    #[test]
    fn after_cycle_uses_binary_codec_for_rising_edges() {
        let mut io = MqttIo::new(
            FakeMqttClient::default(),
            BTreeMap::new(),
            BTreeMap::from([("/output".to_string(), "light".to_string())]),
            MqttPayloadCodec::BinaryBool,
        );
        let mut gv = Gv::default();

        io.init(&mut gv).expect("init should run");
        gv.outputs.insert("light".to_string(), VarValue::Bool(true));
        io.after_cycle(&mut gv).expect("after cycle should run");

        assert_eq!(
            io.client().published,
            vec![MqttMessage::new("/output", vec![0x01])]
        );
    }
}
