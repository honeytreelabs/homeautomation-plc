import pytest

from pathlib import Path
from threading import Thread
import logging
import subprocess
import time


def docker_compose(cmd: str) -> str:
    cwd = Path(__file__).parent.parent / "docker"
    command = f"docker compose {cmd}"
    logging.debug(f"CWD = {cwd}, Command = {command}")
    return subprocess.run(command, check=True, capture_output=True, shell=True, cwd=cwd).stdout.decode('utf-8')


@pytest.fixture(scope="function")
def containers():
    logging.info("Starting containers.")
    docker_compose("up --detach")
    yield
    logging.info("Stopping containers.")
    docker_compose("down --timeout 2 --volumes")


def flaky_mosquitto():
    logging.info("Removing mosquitto.")
    docker_compose("rm -sf mosquitto")
    time.sleep(1)
    logging.info("Re-enabling all services.")
    docker_compose("up -d")


class AutostartThread(Thread):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.start()


def test_mqtt(containers):
    t = AutostartThread(target=flaky_mosquitto)

    logging.info("Executing actual test logic.")

    t.join()
