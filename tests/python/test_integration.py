"""
Integration tests: spin up the real server binary, exercise it via RegisterClient.

Run with:
    SERVER_BINARY=./build/server/register_bridge pytest tests/python/test_integration.py -v
"""
import os
import subprocess
import time
import pytest
from quantumctrl.client import RegisterClient

SERVER_BINARY = os.environ.get("SERVER_BINARY", "./build/server/register_bridge")
PORT = 15666  # different from default to avoid conflicts


@pytest.fixture(scope="module")
def server():
    """Start the server once for the entire test module."""
    proc = subprocess.Popen(
        [SERVER_BINARY, "--port", str(PORT)],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    time.sleep(0.3)  # give the server time to bind
    yield proc
    proc.terminate()
    proc.wait()


@pytest.fixture
def client(server):
    """Fresh connected client per test."""
    c = RegisterClient("localhost", PORT)
    c.connect()
    yield c
    c.disconnect()


def test_write_and_read_back(client):
    client.write(0x10, 12345)
    assert client.read(0x10) == 12345


def test_multiple_independent_registers(client):
    for addr in range(10):
        client.write(addr, addr * 100)
    for addr in range(10):
        assert client.read(addr) == addr * 100


def test_overwrite_register(client):
    client.write(0x20, 1)
    client.write(0x20, 2)
    assert client.read(0x20) == 2


def test_max_value(client):
    client.write(0x01, 0xFFFFFFFF)
    assert client.read(0x01) == 0xFFFFFFFF


def test_zero_value(client):
    client.write(0x02, 99)
    client.write(0x02, 0)
    assert client.read(0x02) == 0
