#include <gtest/gtest.h>
#include "protocol.hpp"

TEST(ProtocolTest, PackedSizeIsSevenBytes) {
    Packet p{Opcode::READ, 0x0001, 0};
    auto buf = pack(p);
    EXPECT_EQ(buf.size(), 7u);
}

TEST(ProtocolTest, OpcodeIsFirstByte) {
    Packet p{Opcode::ACK, 0x0000, 0};
    auto buf = pack(p);
    EXPECT_EQ(buf[0], 0x03);
}

TEST(ProtocolTest, PackUnpackRoundTrip) {
    Packet p{Opcode::WRITE, 0x00A0, 0xDEADBEEF};
    auto buf = pack(p);
    Packet result = unpack(buf);
    EXPECT_EQ(result.opcode, p.opcode);
    EXPECT_EQ(result.address, p.address);
    EXPECT_EQ(result.value, p.value);
}

TEST(ProtocolTest, BigEndianByteOrder) {
    Packet p{Opcode::READ, 0x0102, 0x03040506};
    auto buf = pack(p);
    EXPECT_EQ(buf[1], 0x01);  // high byte of address
    EXPECT_EQ(buf[2], 0x02);  // low byte of address
    EXPECT_EQ(buf[3], 0x03);  // high byte of value
    EXPECT_EQ(buf[6], 0x06);  // low byte of value
}
