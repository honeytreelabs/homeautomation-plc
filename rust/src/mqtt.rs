use std::collections::BTreeMap;

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
                    anyhow::bail!(
                        "MQTT output variable '{var_name}' must be bool, got {other:?}"
                    )
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
        gv.inputs
            .insert("button".to_string(), VarValue::Bool(true));
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

        gv.outputs
            .insert("light".to_string(), VarValue::Bool(true));
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
        gv.outputs
            .insert("light".to_string(), VarValue::Bool(true));
        io.after_cycle(&mut gv).expect("after cycle should run");

        assert_eq!(
            io.client().published,
            vec![MqttMessage::new("/output", vec![0x01])]
        );
    }
}
