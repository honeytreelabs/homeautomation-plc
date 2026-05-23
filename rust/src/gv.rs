use std::collections::BTreeMap;

use serde::Deserialize;

#[derive(Debug, Clone, PartialEq, Eq, Deserialize)]
#[serde(untagged)]
pub enum VarValue {
    Bool(bool),
    Int(i64),
}

#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct Gv {
    pub inputs: BTreeMap<String, VarValue>,
    pub outputs: BTreeMap<String, VarValue>,
}
