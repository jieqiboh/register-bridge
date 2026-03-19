import pytest
import socket
from unittest.mock import MagicMock, patch
from quantumctrl.client import RegisterClient
from quantumctrl.protocol import Opcode, pack_request


def _ack(address: int, value: int) -> bytes:
    return pack_request(Opcode.ACK, address, value)


@pytest.fixture
def client_with_mock_socket():
    """Return an RegisterClient with a pre-attached mock socket."""
    client = RegisterClient("localhost", 5555)
    mock_sock = MagicMock()
    client._sock = mock_sock
    return client, mock_sock


def test_read_returns_value(client_with_mock_socket):
    client, mock_sock = client_with_mock_socket
    mock_sock.recv.return_value = _ack(0x10, 42)
    assert client.read(0x10) == 42


def test_write_sends_correct_packet(client_with_mock_socket):
    client, mock_sock = client_with_mock_socket
    mock_sock.recv.return_value = _ack(0x10, 0)
    client.write(0x10, 99)
    sent = mock_sock.sendall.call_args[0][0]
    from quantumctrl.protocol import unpack_response
    opcode, address, value = unpack_response(sent)
    assert opcode == Opcode.WRITE
    assert address == 0x10
    assert value == 99


def test_read_raises_on_err_response(client_with_mock_socket):
    client, mock_sock = client_with_mock_socket
    mock_sock.recv.return_value = pack_request(Opcode.ERR, 0x10, 0)
    with pytest.raises(RuntimeError):
        client.read(0x10)


def test_write_raises_on_err_response(client_with_mock_socket):
    client, mock_sock = client_with_mock_socket
    mock_sock.recv.return_value = pack_request(Opcode.ERR, 0x10, 0)
    with pytest.raises(RuntimeError):
        client.write(0x10, 1)


def test_connect_opens_socket():
    client = RegisterClient("localhost", 5555)
    with patch("socket.socket") as mock_cls:
        mock_sock = MagicMock()
        mock_cls.return_value = mock_sock
        client.connect()
        mock_sock.connect.assert_called_once_with(("localhost", 5555))


def test_disconnect_closes_socket(client_with_mock_socket):
    client, mock_sock = client_with_mock_socket
    client.disconnect()
    mock_sock.close.assert_called_once()
    assert client._sock is None


def test_disconnect_is_idempotent():
    client = RegisterClient("localhost", 5555)
    client.disconnect()  # should not raise when not connected
