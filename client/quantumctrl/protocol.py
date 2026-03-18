import struct
from enum import IntEnum

PACKET_FORMAT = ">BHI"  # big-endian: uint8, uint16, uint32
PACKET_SIZE = 7


class Opcode(IntEnum):
    """Wire protocol opcodes."""

    READ  = 0x01
    WRITE = 0x02
    ACK   = 0x03
    ERR   = 0x04


def pack_request(opcode: int, address: int, value: int = 0) -> bytes:
    """Pack a request into 7 wire bytes."""
    return struct.pack(PACKET_FORMAT, opcode, address, value)


def unpack_response(data: bytes) -> tuple[int, int, int]:
    """Unpack 7 wire bytes into (opcode, address, value)."""
    opcode, address, value = struct.unpack(PACKET_FORMAT, data)
    return opcode, address, value
