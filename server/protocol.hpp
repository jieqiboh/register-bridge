#pragma once
#include <array>
#include <cstdint>

constexpr size_t PACKET_SIZE = 7;

enum class Opcode : uint8_t {
    READ  = 0x01,
    WRITE = 0x02,
    ACK   = 0x03,
    ERR   = 0x04,
};

struct Packet {
    Opcode   opcode;
    uint16_t address;
    uint32_t value;
};

inline std::array<uint8_t, PACKET_SIZE> pack(const Packet& p) {
    std::array<uint8_t, PACKET_SIZE> buf{};
    buf[0] = static_cast<uint8_t>(p.opcode);
    buf[1] = (p.address >> 8) & 0xFF;
    buf[2] =  p.address       & 0xFF;
    buf[3] = (p.value >> 24)  & 0xFF;
    buf[4] = (p.value >> 16)  & 0xFF;
    buf[5] = (p.value >>  8)  & 0xFF;
    buf[6] =  p.value         & 0xFF;
    return buf;
}

inline Packet unpack(const std::array<uint8_t, PACKET_SIZE>& buf) {
    Packet p;
    p.opcode  = static_cast<Opcode>(buf[0]);
    p.address = (static_cast<uint16_t>(buf[1]) << 8) | buf[2];
    p.value   = (static_cast<uint32_t>(buf[3]) << 24) |
                (static_cast<uint32_t>(buf[4]) << 16) |
                (static_cast<uint32_t>(buf[5]) <<  8) |
                buf[6];
    return p;
}
