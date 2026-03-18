#pragma once
#include <array>
#include <cstdint>
#include <mutex>

class RegisterBank {
public:
    static constexpr size_t NUM_REGISTERS = 256;

    /**
     * Read the value at the given address.
     * Throws std::out_of_range if address >= 256.
     */
    uint32_t read(uint16_t address);

    /**
     * Write value to the given address.
     * Throws std::out_of_range if address >= 256.
     */
    void write(uint16_t address, uint32_t value);

private:
    std::array<uint32_t, NUM_REGISTERS> registers_{};
    std::mutex mutex_;
};
