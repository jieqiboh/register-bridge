from quantumctrl.protocol import pack_request, unpack_response, Opcode, PACKET_SIZE


def test_pack_size():
    assert len(pack_request(Opcode.READ, 0x0010)) == PACKET_SIZE


def test_pack_opcode_is_first_byte():
    data = pack_request(Opcode.ACK, 0, 0)
    assert data[0] == 0x03


def test_roundtrip():
    data = pack_request(Opcode.WRITE, 0x00A0, 0xDEADBEEF)
    opcode, address, value = unpack_response(data)
    assert opcode == Opcode.WRITE
    assert address == 0x00A0
    assert value == 0xDEADBEEF


def test_big_endian_byte_order():
    data = pack_request(Opcode.READ, 0x0102, 0x03040506)
    assert data[1] == 0x01  # high byte of address
    assert data[2] == 0x02
    assert data[3] == 0x03  # high byte of value
    assert data[6] == 0x06
