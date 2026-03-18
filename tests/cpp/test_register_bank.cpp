#include <gtest/gtest.h>
#include <stdexcept>
#include <thread>
#include <vector>
#include "register_bank.hpp"

TEST(RegisterBankTest, ReadUninitializedReturnsZero) {
    RegisterBank bank;
    EXPECT_EQ(bank.read(0), 0u);
}

TEST(RegisterBankTest, WriteAndReadBack) {
    RegisterBank bank;
    bank.write(0x10, 42u);
    EXPECT_EQ(bank.read(0x10), 42u);
}

TEST(RegisterBankTest, IndependentRegisters) {
    RegisterBank bank;
    bank.write(0x01, 1u);
    bank.write(0x02, 2u);
    EXPECT_EQ(bank.read(0x01), 1u);
    EXPECT_EQ(bank.read(0x02), 2u);
}

TEST(RegisterBankTest, OutOfRangeReadThrows) {
    RegisterBank bank;
    EXPECT_THROW(bank.read(256), std::out_of_range);
}

TEST(RegisterBankTest, OutOfRangeWriteThrows) {
    RegisterBank bank;
    EXPECT_THROW(bank.write(256, 0), std::out_of_range);
}

TEST(RegisterBankTest, ConcurrentWritesDoNotCrash) {
    RegisterBank bank;
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&bank, i] {
            for (int j = 0; j < 1000; ++j)
                bank.write(static_cast<uint16_t>(i), static_cast<uint32_t>(j));
        });
    }
    for (auto& t : threads) t.join();
    // No crash = mutex is working
}
