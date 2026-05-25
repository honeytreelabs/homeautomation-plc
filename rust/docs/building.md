# Building The Rust Runtime

This crate is the starting point for the Rust `homeautomation-plc` runtime.
It is intentionally separate from the C/C++ reference implementation in the
repository root.

The Rust runtime uses TOML as its native configuration format. Existing YAML
reference configs can be converted with Mike Farah's `yq` and then hand-polished:

```sh
yq -o toml '.' ../examples/generic-inline/generic-inline.yaml > examples/generic-inline.toml
```

## Native

```sh
cd rust
cargo test
cargo run -- --config examples/generic-inline.toml
```

From the repository root, the same native checks are available through:

```sh
make rust-lock
make rust-test
make rust-build
```

## Raspberry Pi / OpenWrt Targets

The planned targets are:

- `armv7-unknown-linux-musleabihf` for Raspberry Pi 2 and 32-bit images
- `aarch64-unknown-linux-musl` for 64-bit Raspberry Pi 3/4 images

For the current pure-Rust skeleton, Cargo uses Rust's bundled `rust-lld`
linker for both musl targets through `.cargo/config.toml`:

```sh
cd rust
cargo build --release --target armv7-unknown-linux-musleabihf
cargo build --release --target aarch64-unknown-linux-musl
```

Repository-root Make targets:

```sh
make rust-build-rpi2
make rust-build-rpi3
```

If a later dependency needs a C compiler during cross-compilation, such as
vendored Lua through `mlua`, use `cross` or set Cargo's target-specific linker
environment variable to a matching musl cross toolchain, for example:

```sh
export CARGO_TARGET_ARMV7_UNKNOWN_LINUX_MUSLEABIHF_LINKER=arm-linux-musleabihf-gcc
export CARGO_TARGET_AARCH64_UNKNOWN_LINUX_MUSL_LINKER=aarch64-linux-musl-gcc
```

With `cross`:

```sh
cd rust
cross build --release --target armv7-unknown-linux-musleabihf
cross build --release --target aarch64-unknown-linux-musl
```

`Cross.toml` intentionally does not pin target images. Letting `cross` select
the default image keeps the image tag aligned with the installed `cross`
version.

Repository-root Make targets:

```sh
make rust-cross-rpi2
make rust-cross-rpi3
```

If switching between native Cargo and `cross` leaves stale artifacts in the
shared target directory, clean once and rebuild:

```sh
rm -rf rust/target
make rust-cross-rpi3
```

The first runtime milestone should verify these builds again after adding
`mlua` with vendored Lua.

## Raspberry Pi 2 Lua Smoke Test

`examples/rpi-lua-smoke.toml` is a no-IO config for checking that the Rust
runtime and embedded Lua interpreter run on a Raspberry Pi 2. It runs one Lua
task per second, prints from `Init` and `Cycle`, and exercises `R_TRIG`,
`F_TRIG`, and `to_millis_since_start`.

Build the Pi 2 binary from the repository root:

```sh
make rust-build-rpi2
```

Copy the binary and config to the Pi:

```sh
scp rust/target/armv7-unknown-linux-musleabihf/release/homeautomation-plc \
  root@raspberrypi:/tmp/homeautomation-plc
scp rust/examples/rpi-lua-smoke.toml \
  root@raspberrypi:/tmp/rpi-lua-smoke.toml
```

Run it on the Pi:

```sh
ssh root@raspberrypi
chmod +x /tmp/homeautomation-plc
RUST_LOG=info /tmp/homeautomation-plc --config /tmp/rpi-lua-smoke.toml
```

Expected output includes:

```text
Lua Init: embedded interpreter is running
Lua Cycle: tick=1 now_ms=... signal=true rising=true falling=false
Lua Cycle: tick=2 now_ms=... signal=false rising=false falling=true
```

Stop it with Ctrl-C.

## Built-In Lua Components

The Rust runtime currently registers these Lua components before loading user
scripts:

- `R_TRIG([last])`
- `F_TRIG([last])`
- `to_millis_since_start(now)`
- `BlindConfigFromMillis(periodIdle, periodUp, periodDown)`
- `Blind.new(config)`

`Blind` follows the C++ reference API shape:

```lua
function Init(gv)
  blind = Blind.new(BlindConfigFromMillis(500, 30000, 30000))
end

function Cycle(gv, now)
  gv.outputs.blind_up, gv.outputs.blind_down =
    blind:execute(now, gv.inputs.button_up, gv.inputs.button_down)
end
```

## MQTT IO

MQTT IO is implemented behind a Rust `MqttClient` trait and covered with
fake-client tests. The generic runtime wires `type = "mqtt"` TOML entries to a
real TCP MQTT client using `rumqttc` without its default TLS features, keeping
the first network backend friendly to mostly static musl builds.

The backend preserves the C++ reference behavior:

- subscribe to configured input topics during initialization
- before each cycle, reset mapped MQTT inputs to `false`
- drain received messages and map valid payloads into `gv.inputs`
- after each cycle, publish only rising edges for configured outputs

The real client runs the MQTT event loop on a background thread. Incoming
publish packets are drained by the PLC task before each cycle; outgoing
subscribe/publish requests are sent through the client handle. TCP broker URLs
using `tcp://` or `mqtt://` are supported in the first version.

MQTT payloads are handled through an explicit codec:

- `text-bool`: `true` is payload bytes `1`, `false` is payload bytes `0`
- `binary-bool`: `true` is single byte `0x01`, `false` is single byte `0x00`

`text-bool` is the compatibility default for the C++ reference behavior.

The TOML shape is:

```toml
[[tasks.io]]
type = "mqtt"
payload_codec = "text-bool"

[tasks.io.client]
address = "tcp://localhost:1883"
client_id = "generic::main"
# username = "user"
# password = "password"

[tasks.io.inputs]
"/homeautomation/light_remote" = "light_remote"

[tasks.io.outputs]
"/homeautomation/light" = "light"
```

Alternatively, the client can be configured as separate host/port fields:

```toml
[tasks.io.client]
host = "localhost"
port = 1883
client_id = "generic::main"
```

For automated native and QEMU smoke tests from the repository root, run:

```sh
make rust-mqtt-smoke
```

The target runs the parametrized pytest-based smoke test in `integration_test/`
for native, Raspberry Pi 2 via `qemu-arm`, and Raspberry Pi 3 via
`qemu-aarch64`. Each case starts Mosquitto in Docker or Podman, builds the Rust
binary, starts the PLC with `rust/examples/mqtt-smoke.toml`, publishes `1` to
`/homeautomation/smoke/button`, and verifies that the runtime publishes `1` to
`/homeautomation/smoke/light`.

Platform-specific targets are also available:

```sh
make rust-qemu-mqtt-smoke-rpi2
make rust-qemu-mqtt-smoke-rpi3
```

The native case uses `cargo`. The QEMU cases use `cross` for building and
require `qemu-arm` or `qemu-aarch64` on the host.

For a manual smoke test, start an MQTT broker and run:

```sh
homeautomation-plc --config rust/examples/mqtt-smoke.toml
```

Then publish an input edge from another shell:

```sh
mosquitto_pub -h localhost -t /homeautomation/smoke/button -m 1
```

The runtime should publish a rising output edge to
`/homeautomation/smoke/light`.

## I2C IO

I2C IO is implemented behind an `I2cBus` trait so device behavior can be tested
with fake buses. The runtime wires `type = "i2c"` TOML entries to a Linux
`/dev/i2c-*` bus.

Supported components:

- `pcf8574` input
- `pcf8574` output
- `max7311` output

The backend preserves the C++ reference behavior:

- input and output device bits are inverted
- missing mapped global variables are created as boolean values during IO init
- inputs are copied into `gv.inputs` before each cycle
- outputs are copied from `gv.outputs` after each cycle
- output devices write only when their effective output byte changed

TOML shape:

```toml
[[tasks.io]]
type = "i2c"
bus = "/dev/i2c-1"

[tasks.io.components."0x3b"]
type = "pcf8574"
direction = "input"
inputs = { 0 = "button_up", 1 = "button_down" }

[tasks.io.components."0x20"]
type = "max7311"
direction = "output"
outputs = { 0 = "blind_up", 1 = "blind_down" }
```

## Modbus RTU IO

Modbus RTU IO is implemented behind a `ModbusClient` trait so device behavior
can be tested with fake clients. The real serial client is intentionally pending;
the first implementation wires config and device behavior through the runtime
and returns a clear runtime error if the real backend is initialized.

Supported components:

- `WP8026ADAM` input
- `R4S8CRMB` output

The backend preserves the C++ reference behavior:

- `WP8026ADAM` reads 8 input bits starting at address `0x0008`
- `R4S8CRMB` writes 8 output coils starting at address `0x0000`
- missing mapped global variables are created as boolean values during IO init
- inputs are copied into `gv.inputs` before each cycle
- outputs are copied from `gv.outputs` after each cycle
- output coils are written only when their effective output array changed

TOML shape:

```toml
[[tasks.io]]
type = "modbus-rtu"
path = "/dev/ttyUSB0"
baud = 9600
data_bit = 8
parity = "N"
stop_bit = 1

[[tasks.io.components]]
type = "WP8026ADAM"
slave = 1
inputs = { 0 = "button_1", 1 = "button_2" }

[[tasks.io.components]]
type = "R4S8CRMB"
slave = 1
outputs = { 0 = "relay_1", 1 = "relay_2" }
```

The likely real backend direction is `tokio-modbus` with
`default-features = false` and `features = ["rtu-sync"]`, so the PLC scheduler
can keep its synchronous IO lifecycle.
