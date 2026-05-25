import logging
import shutil
import socket
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parents[1]
RUST_DIR = REPO_ROOT / "rust"
BROKER_IMAGE = "eclipse-mosquitto:2"
BROKER_NAME = "homeautomation-plc-rust-mqtt-smoke"
INPUT_TOPIC = "/homeautomation/smoke/button"
OUTPUT_TOPIC = "/homeautomation/smoke/light"


@dataclass(frozen=True)
class PlcRuntime:
    name: str
    build_command: list[str]
    run_command: list[str]
    required_commands: list[str]


RUNTIMES = [
    PlcRuntime(
        name="native",
        build_command=[
            "cargo",
            "build",
            "--manifest-path",
            str(RUST_DIR / "Cargo.toml"),
        ],
        run_command=[str(RUST_DIR / "target/debug/homeautomation-plc")],
        required_commands=["cargo"],
    ),
    PlcRuntime(
        name="qemu-rpi2",
        build_command=[
            "cross",
            "build",
            "--release",
            "--target",
            "armv7-unknown-linux-musleabihf",
            "--manifest-path",
            str(RUST_DIR / "Cargo.toml"),
        ],
        run_command=[
            "qemu-arm",
            str(
                RUST_DIR
                / "target/armv7-unknown-linux-musleabihf/release/homeautomation-plc"
            ),
        ],
        required_commands=["cross", "qemu-arm"],
    ),
    PlcRuntime(
        name="qemu-rpi3",
        build_command=[
            "cross",
            "build",
            "--release",
            "--target",
            "aarch64-unknown-linux-musl",
            "--manifest-path",
            str(RUST_DIR / "Cargo.toml"),
        ],
        run_command=[
            "qemu-aarch64",
            str(
                RUST_DIR
                / "target/aarch64-unknown-linux-musl/release/homeautomation-plc"
            ),
        ],
        required_commands=["cross", "qemu-aarch64"],
    ),
]


def free_tcp_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def command_exists(command: str) -> bool:
    return shutil.which(command) is not None


def run(command: list[str], **kwargs) -> subprocess.CompletedProcess[str]:
    logging.info("running: %s", " ".join(command))
    return subprocess.run(
        command,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        **kwargs,
    )


def wait_for_broker(container_runtime: str) -> None:
    for _ in range(30):
        probe = subprocess.run(
            [
                container_runtime,
                "exec",
                BROKER_NAME,
                "mosquitto_pub",
                "-h",
                "127.0.0.1",
                "-t",
                "/homeautomation/smoke/ready",
                "-m",
                "1",
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        if probe.returncode == 0:
            return
        time.sleep(0.2)

    raise AssertionError("Mosquitto container did not become ready")


@pytest.fixture(scope="module", name="container_runtime")
def fixture_container_runtime() -> str:
    for command in ("docker", "podman"):
        if command_exists(command):
            return command
    pytest.skip("docker or podman is required for the Rust MQTT smoke test")


@pytest.fixture(scope="module", name="broker_port")
def fixture_broker_port() -> int:
    return free_tcp_port()


@pytest.fixture
def plc_runtime(request) -> PlcRuntime:
    runtime = request.param
    require_commands(runtime.required_commands)
    return runtime


@pytest.fixture(scope="module", name="mosquitto_broker")
def fixture_mosquitto_broker(
    container_runtime: str,
    broker_port: int,
    tmp_path_factory: pytest.TempPathFactory,
):
    tmp_path = tmp_path_factory.mktemp("mqtt-broker")
    config = tmp_path / "mosquitto.conf"
    config.write_text("listener 1883 0.0.0.0\nallow_anonymous true\n", encoding="utf-8")

    subprocess.run(
        [container_runtime, "rm", "-f", BROKER_NAME],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    )

    try:
        run(
            [
                container_runtime,
                "run",
                "-d",
                "--name",
                BROKER_NAME,
                "-p",
                f"{broker_port}:1883",
                "-v",
                f"{config}:/mosquitto/config/mosquitto.conf:ro",
                BROKER_IMAGE,
                "mosquitto",
                "-c",
                "/mosquitto/config/mosquitto.conf",
            ]
        )
        wait_for_broker(container_runtime)
        yield
    finally:
        subprocess.run(
            [container_runtime, "rm", "-f", BROKER_NAME],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )


def write_smoke_config(tmp_path: Path, broker_port: int, runtime_name: str) -> Path:
    source = (RUST_DIR / "examples/mqtt-smoke.toml").read_text(encoding="utf-8")
    config = source.replace(
        "tcp://localhost:1883",
        f"tcp://127.0.0.1:{broker_port}",
    ).replace(
        'client_id = "homeautomation-plc-smoke"',
        f'client_id = "homeautomation-plc-smoke-{runtime_name}-{broker_port}"',
    )

    path = tmp_path / "mqtt-smoke.toml"
    path.write_text(config, encoding="utf-8")
    return path


def require_commands(commands: list[str]) -> None:
    missing = [command for command in commands if not command_exists(command)]
    if missing:
        pytest.skip(f"missing required command(s): {', '.join(missing)}")


@pytest.mark.smoketest
@pytest.mark.parametrize(
    "plc_runtime",
    RUNTIMES,
    ids=[runtime.name for runtime in RUNTIMES],
    indirect=True,
)
def test_rust_mqtt_smoke(
    plc_runtime: PlcRuntime,
    container_runtime: str,
    broker_port: int,
    mosquitto_broker,
    tmp_path,
):
    del mosquitto_broker  # just make sure to use fixture

    smoke_config = write_smoke_config(tmp_path, broker_port, plc_runtime.name)

    run(plc_runtime.build_command, cwd=RUST_DIR)

    plc_log = tmp_path / "plc.log"
    with plc_log.open("w", encoding="utf-8") as log:
        plc_process = subprocess.Popen(
            [*plc_runtime.run_command, "--config", str(smoke_config)],
            cwd=REPO_ROOT,
            stdout=log,
            stderr=subprocess.STDOUT,
            text=True,
        )

    try:
        time.sleep(2)
        if plc_process.poll() is not None:
            pytest.fail(
                "PLC runtime exited before MQTT smoke publish\n"
                f"{plc_log.read_text(encoding='utf-8')}"
            )

        subscriber = subprocess.Popen(
            [
                container_runtime,
                "exec",
                BROKER_NAME,
                "mosquitto_sub",
                "-h",
                "127.0.0.1",
                "-t",
                OUTPUT_TOPIC,
                "-C",
                "1",
                "-W",
                "15",
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        time.sleep(0.5)
        run(
            [
                container_runtime,
                "exec",
                BROKER_NAME,
                "mosquitto_pub",
                "-h",
                "127.0.0.1",
                "-t",
                INPUT_TOPIC,
                "-m",
                "1",
            ]
        )

        stdout, stderr = subscriber.communicate(timeout=20)
        assert subscriber.returncode == 0, (
            f"subscriber failed with {subscriber.returncode}\n"
            f"stderr:\n{stderr}\n"
            f"plc log:\n{plc_log.read_text(encoding='utf-8')}"
        )
        assert stdout.strip() == "1"
    finally:
        plc_process.terminate()
        try:
            plc_process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            plc_process.kill()
            plc_process.wait(timeout=5)
