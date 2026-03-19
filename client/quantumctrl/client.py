import socket
from .protocol import Opcode, PACKET_SIZE, pack_request, unpack_response


class RegisterClient:
    def __init__(self, host: str, port: int) -> None:
        self._host = host
        self._port = port
        self._sock: socket.socket | None = None

    def connect(self) -> None:
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.connect((self._host, self._port))

    def disconnect(self) -> None:
        if self._sock is not None:
            self._sock.close()
            self._sock = None

    def read(self, address: int) -> int:
        self._send(pack_request(Opcode.READ, address))
        return self._recv_value()

    def write(self, address: int, value: int) -> None:
        self._send(pack_request(Opcode.WRITE, address, value))
        self._recv_ack()

    def _send(self, data: bytes) -> None:
        assert self._sock is not None, "Not connected"
        self._sock.sendall(data)

    def _recv(self) -> tuple[int, int, int]:
        assert self._sock is not None, "Not connected"
        data = self._sock.recv(PACKET_SIZE)
        return unpack_response(data)

    def _recv_value(self) -> int:
        opcode, _, value = self._recv()
        if opcode != Opcode.ACK:
            raise RuntimeError(f"Expected ACK, got {opcode:#04x}")
        return value

    def _recv_ack(self) -> None:
        opcode, _, _ = self._recv()
        if opcode != Opcode.ACK:
            raise RuntimeError(f"Expected ACK, got {opcode:#04x}")
