#include "register_bank.hpp"
#include <stdexcept>

uint32_t RegisterBank::read(uint16_t address) {
    if (address >= NUM_REGISTERS)
        throw std::out_of_range("Register address out of range");
    std::lock_guard<std::mutex> lock(mutex_);
    return registers_[address];
}

void RegisterBank::write(uint16_t address, uint32_t value) {
    if (address >= NUM_REGISTERS)
        throw std::out_of_range("Register address out of range");
    std::lock_guard<std::mutex> lock(mutex_);
    registers_[address] = value;
}
