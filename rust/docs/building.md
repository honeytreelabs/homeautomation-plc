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
