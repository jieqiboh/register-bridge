# FPGA Register Bridge Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build a minimal FPGA register interface simulator with a C++ TCP server and a Python client library.

**Architecture:** A C++ server holds a 256-register bank, listens on TCP, and speaks a 7-byte binary protocol. A Python library wraps the socket in a clean `read(addr)`/`write(addr, value)` API. Split into 7 independently committable parts.

**Tech Stack:** C++17, CMake 3.16+, GoogleTest (via FetchContent), Python 3.11, pytest, setuptools

---

## Part 1: Project Skeleton

**Goal:** Create the directory structure and CMake build. Everything compiles (empty executables). No tests yet.

**Files:**
- Create: `CMakeLists.txt`
- Create: `server/CMakeLists.txt`
- Create: `server/main.cpp`
- Create: `tests/cpp/CMakeLists.txt`
- Create: `client/setup.py`
- Create: `client/quantumctrl/__init__.py`

### Step 1: Create root CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(fpga_bridge VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(server)
add_subdirectory(tests/cpp)
```

### Step 2: Create server/CMakeLists.txt

```cmake
add_library(fpga_bridge_lib
    register_bank.cpp
    thread_pool.cpp
    server.cpp
)
target_include_directories(fpga_bridge_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(fpga_bridge main.cpp)
target_link_libraries(fpga_bridge PRIVATE fpga_bridge_lib)
```

### Step 3: Create placeholder source files

`server/main.cpp` (minimal):
```cpp
int main() { return 0; }
```

`server/register_bank.cpp`, `server/thread_pool.cpp`, `server/server.cpp` — each just an empty file for now (CMake needs them to exist).

### Step 4: Create tests/cpp/CMakeLists.txt

```cmake
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(fpga_bridge_tests
    test_protocol.cpp
    test_register_bank.cpp
    test_thread_pool.cpp
)
target_link_libraries(fpga_bridge_tests
    PRIVATE fpga_bridge_lib GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(fpga_bridge_tests)
```

Create placeholder test files — each containing just `#include <gtest/gtest.h>` with no test cases yet.

### Step 5: Create Python package stubs

`client/setup.py`:
```python
from setuptools import setup, find_packages

setup(
    name="quantumctrl",
    version="0.1.0",
    packages=find_packages(),
    python_requires=">=3.11",
)
```

`client/quantumctrl/__init__.py` — empty for now.

Create empty placeholder files: `client/quantumctrl/protocol.py`, `client/quantumctrl/client.py`.

Also create `tests/python/test_protocol.py`, `tests/python/test_client.py`, `tests/python/test_integration.py` — each empty.

### Step 6: Verify build

```bash
cmake -B build && cmake --build build
```
Expected: compiles successfully.

Install the Python package:
```bash
pip install -e client/
```

### Step 7: Commit

```bash
git init
git add CMakeLists.txt server/ tests/ client/ docs/
git commit -m "feat: project skeleton — cmake build + python package stubs"
```

---

## Part 2: Wire Protocol

**Goal:** Define the binary packet format in both C++ and Python with unit tests on both sides. No networking yet.

**Files:**
- Create: `server/protocol.hpp`
- Modify: `tests/cpp/test_protocol.cpp`
- Modify: `client/quantumctrl/protocol.py`
- Modify: `tests/python/test_protocol.py`

### Step 1: Write failing C++ protocol tests

Replace `tests/cpp/test_protocol.cpp`:
```cpp
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
```

### Step 2: Run to verify failure

```bash
cmake -B build && cmake --build build 2>&1 | head -20
```
Expected: compile error — `protocol.hpp` not found.

### Step 3: Implement server/protocol.hpp

```cpp
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
```

### Step 4: Verify C++ tests pass

```bash
cmake -B build && cmake --build build && cd build && ctest -R ProtocolTest -V
```
Expected: 4 tests pass.

### Step 5: Write failing Python protocol tests

Replace `tests/python/test_protocol.py`:
```python
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
```

Run to verify failure:
```bash
pytest tests/python/test_protocol.py -v
```
Expected: ImportError.

### Step 6: Implement client/quantumctrl/protocol.py

```python
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
```

### Step 7: Verify Python tests pass

```bash
pytest tests/python/test_protocol.py -v
```
Expected: 4 tests pass.

### Step 8: Commit

```bash
git add server/protocol.hpp tests/cpp/test_protocol.cpp \
        client/quantumctrl/protocol.py tests/python/test_protocol.py
git commit -m "feat: wire protocol — pack/unpack for C++ and Python"
```

---

## Part 3: Register Bank

**Goal:** Thread-safe 256-register array with mutex. Unit tested including concurrent access.

**Files:**
- Create: `server/register_bank.hpp`
- Modify: `server/register_bank.cpp`
- Modify: `tests/cpp/test_register_bank.cpp`

### Step 1: Write failing tests

Replace `tests/cpp/test_register_bank.cpp`:
```cpp
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
```

### Step 2: Verify failure

```bash
cmake -B build && cmake --build build 2>&1 | head -20
```
Expected: compile error — `register_bank.hpp` not found.

### Step 3: Create server/register_bank.hpp

```cpp
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
```

### Step 4: Implement server/register_bank.cpp

```cpp
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
```

### Step 5: Verify tests pass

```bash
cmake -B build && cmake --build build && cd build && ctest -R RegisterBankTest -V
```
Expected: 6 tests pass.

### Step 6: Commit

```bash
git add server/register_bank.hpp server/register_bank.cpp \
        tests/cpp/test_register_bank.cpp
git commit -m "feat: thread-safe register bank (256 x uint32)"
```

---

## Part 4: Thread Pool

**Goal:** Fixed-size thread pool with a task queue and condition variable. Unit tested.

**Files:**
- Create: `server/thread_pool.hpp`
- Modify: `server/thread_pool.cpp`
- Modify: `tests/cpp/test_thread_pool.cpp`

### Step 1: Write failing tests

Replace `tests/cpp/test_thread_pool.cpp`:
```cpp
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include "thread_pool.hpp"

TEST(ThreadPoolTest, ExecutesAllTasks) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    for (int i = 0; i < 100; ++i)
        pool.enqueue([&counter] { ++counter; });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(counter.load(), 100);
}

TEST(ThreadPoolTest, DestructorWaitsForTasks) {
    std::atomic<int> counter{0};
    {
        ThreadPool pool(2);
        for (int i = 0; i < 20; ++i)
            pool.enqueue([&counter] { ++counter; });
    }  // destructor blocks until all tasks done
    EXPECT_EQ(counter.load(), 20);
}
```

### Step 2: Verify failure

```bash
cmake -B build && cmake --build build 2>&1 | head -20
```
Expected: compile error — `thread_pool.hpp` not found.

### Step 3: Create server/thread_pool.hpp

```cpp
#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
    /**
     * Start num_threads worker threads, ready to accept tasks.
     */
    explicit ThreadPool(size_t num_threads);

    /**
     * Signal stop and join all threads.
     */
    ~ThreadPool();

    /**
     * Add a task to the queue. One idle worker will pick it up.
     */
    void enqueue(std::function<void()> task);

private:
    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex                        mutex_;
    std::condition_variable           cv_;
    bool                              stop_ = false;
};
```

### Step 4: Implement server/thread_pool.cpp

```cpp
#include "thread_pool.hpp"

ThreadPool::ThreadPool(size_t num_threads) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                    if (stop_ && tasks_.empty()) return;
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    for (auto& w : workers_) w.join();
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(std::move(task));
    }
    cv_.notify_one();
}
```

### Step 5: Verify tests pass

```bash
cmake -B build && cmake --build build && cd build && ctest -R ThreadPoolTest -V
```
Expected: 2 tests pass.

### Step 6: Commit

```bash
git add server/thread_pool.hpp server/thread_pool.cpp \
        tests/cpp/test_thread_pool.cpp
git commit -m "feat: fixed 4-thread pool with condition-variable task queue"
```

---

## Part 5: TCP Server

**Goal:** Wire the thread pool and register bank together with a TCP accept loop. Manual smoke test only (integration tests come in Part 7).

**Files:**
- Create: `server/server.hpp`
- Modify: `server/server.cpp`
- Modify: `server/main.cpp`

### Step 1: Create server/server.hpp

```cpp
#pragma once
#include "register_bank.hpp"
#include "thread_pool.hpp"

class Server {
public:
    /**
     * Create a server that will listen on the given port.
     * Does not bind or accept until run() is called.
     */
    explicit Server(int port, size_t num_threads = 4);

    /**
     * Bind, listen, and block accepting client connections.
     */
    void run();

    /**
     * Signal the accept loop to stop.
     */
    void stop();

private:
    void handle_client(int client_fd);

    int          port_;
    int          server_fd_ = -1;
    bool         running_   = false;
    ThreadPool   pool_;
    RegisterBank bank_;
};
```

### Step 2: Implement server/server.cpp

```cpp
#include "server.hpp"
#include "protocol.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <iostream>

Server::Server(int port, size_t num_threads)
    : port_(port), pool_(num_threads) {}

void Server::run() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(static_cast<uint16_t>(port_));

    bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    listen(server_fd_, 10);

    running_ = true;
    std::cout << "Listening on port " << port_ << "\n";

    while (running_) {
        int client_fd = accept(server_fd_, nullptr, nullptr);
        if (client_fd < 0) break;
        pool_.enqueue([this, client_fd]() { handle_client(client_fd); });
    }
}

void Server::stop() {
    running_ = false;
    if (server_fd_ >= 0) close(server_fd_);
}

void Server::handle_client(int client_fd) {
    std::array<uint8_t, PACKET_SIZE> buf{};

    while (true) {
        ssize_t n = recv(client_fd, buf.data(), PACKET_SIZE, MSG_WAITALL);
        if (n != static_cast<ssize_t>(PACKET_SIZE)) break;

        Packet req  = unpack(buf);
        Packet resp{};
        resp.address = req.address;

        if (req.opcode == Opcode::READ) {
            resp.opcode = Opcode::ACK;
            resp.value  = bank_.read(req.address);
        } else if (req.opcode == Opcode::WRITE) {
            bank_.write(req.address, req.value);
            resp.opcode = Opcode::ACK;
            resp.value  = 0;
        } else {
            resp.opcode = Opcode::ERR;
            resp.value  = 0;
        }

        auto out = pack(resp);
        send(client_fd, out.data(), PACKET_SIZE, 0);
    }

    close(client_fd);
}
```

### Step 3: Implement server/main.cpp

```cpp
#include <cstdlib>
#include <iostream>
#include <string>
#include "server.hpp"

int main(int argc, char* argv[]) {
    int port = 5555;
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--port")
            port = std::atoi(argv[i + 1]);
    }

    Server server(port);
    server.run();
    return 0;
}
```

### Step 4: Build and smoke test

```bash
cmake -B build && cmake --build build
./build/server/fpga_bridge --port 5555 &
```

In another terminal, send a raw packet with Python:
```python
import socket, struct
s = socket.socket()
s.connect(("localhost", 5555))
s.sendall(struct.pack(">BHI", 0x02, 0x10, 42))   # WRITE addr=0x10 val=42
s.sendall(struct.pack(">BHI", 0x01, 0x10, 0))    # READ  addr=0x10
print(struct.unpack(">BHI", s.recv(7)))            # expect (0x03, 0x10, 42)
s.close()
```

### Step 5: Commit

```bash
kill %1  # stop background server
git add server/server.hpp server/server.cpp server/main.cpp
git commit -m "feat: tcp server — accept loop wired to thread pool and register bank"
```

---

## Part 6: Python Client

**Goal:** Clean Python client API. All public methods unit-tested with a mocked socket.

**Files:**
- Modify: `client/quantumctrl/client.py`
- Modify: `client/quantumctrl/__init__.py`
- Modify: `tests/python/test_client.py`

### Step 1: Write failing tests

Replace `tests/python/test_client.py`:
```python
import pytest
import socket
from unittest.mock import MagicMock, patch
from quantumctrl.client import FPGAClient
from quantumctrl.protocol import Opcode, pack_request


def _ack(address: int, value: int) -> bytes:
    return pack_request(Opcode.ACK, address, value)


@pytest.fixture
def client_with_mock_socket():
    """Return an FPGAClient with a pre-attached mock socket."""
    client = FPGAClient("localhost", 5555)
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
    client = FPGAClient("localhost", 5555)
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
    client = FPGAClient("localhost", 5555)
    client.disconnect()  # should not raise when not connected
```

### Step 2: Verify failure

```bash
pytest tests/python/test_client.py -v 2>&1 | head -20
```
Expected: ImportError.

### Step 3: Implement client/quantumctrl/client.py

```python
import socket
from .protocol import Opcode, PACKET_SIZE, pack_request, unpack_response


class FPGAClient:
    """Client for the FPGA register bridge server.

    Usage::

        client = FPGAClient("localhost", 5555)
        client.connect()
        client.write(0x10, 42)
        value = client.read(0x10)  # 42
        client.disconnect()
    """

    def __init__(self, host: str, port: int) -> None:
        self._host = host
        self._port = port
        self._sock: socket.socket | None = None

    def connect(self) -> None:
        """Open a TCP connection to the server."""
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.connect((self._host, self._port))

    def disconnect(self) -> None:
        """Close the TCP connection."""
        if self._sock is not None:
            self._sock.close()
            self._sock = None

    def read(self, address: int) -> int:
        """Read the 32-bit value at the given register address."""
        self._send(pack_request(Opcode.READ, address))
        return self._recv_value()

    def write(self, address: int, value: int) -> None:
        """Write a 32-bit value to the given register address."""
        self._send(pack_request(Opcode.WRITE, address, value))
        self._recv_ack()

    # ------------------------------------------------------------------ #
    # Private helpers                                                      #
    # ------------------------------------------------------------------ #

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
```

### Step 4: Update client/quantumctrl/__init__.py

```python
from .client import FPGAClient

__all__ = ["FPGAClient"]
```

### Step 5: Verify all Python tests pass

```bash
pytest tests/python/test_protocol.py tests/python/test_client.py -v
```
Expected: all tests pass.

### Step 6: Commit

```bash
git add client/quantumctrl/client.py client/quantumctrl/__init__.py \
        tests/python/test_client.py
git commit -m "feat: python client — FPGAClient with read/write API"
```

---

## Part 7: Integration Tests

**Goal:** End-to-end tests that spin up the real server binary and talk to it via the Python client.

**Files:**
- Modify: `tests/python/test_integration.py`

### Step 1: Write the tests

Replace `tests/python/test_integration.py`:
```python
"""
Integration tests: spin up the real server binary, exercise it via FPGAClient.

Run with:
    SERVER_BINARY=./build/server/fpga_bridge pytest tests/python/test_integration.py -v
"""
import os
import subprocess
import time
import pytest
from quantumctrl.client import FPGAClient

SERVER_BINARY = os.environ.get("SERVER_BINARY", "./build/server/fpga_bridge")
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
    c = FPGAClient("localhost", PORT)
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
```

### Step 2: Run integration tests

```bash
cmake -B build && cmake --build build
SERVER_BINARY=./build/server/fpga_bridge pytest tests/python/test_integration.py -v
```
Expected: 5 tests pass.

### Step 3: Run full test suite

```bash
cd build && ctest -V   # C++ tests
cd ..
pytest tests/python/   # Python unit + integration tests
```
Expected: all tests pass.

### Step 4: Commit

```bash
git add tests/python/test_integration.py
git commit -m "feat: integration tests — end-to-end read/write via real server"
```

---

## Summary

| Part | What you build | Commit message |
|------|---------------|----------------|
| 1 | Project skeleton | `feat: project skeleton` |
| 2 | Wire protocol (C++ + Python) | `feat: wire protocol` |
| 3 | Register bank | `feat: thread-safe register bank` |
| 4 | Thread pool | `feat: thread pool` |
| 5 | TCP server | `feat: tcp server` |
| 6 | Python client | `feat: python client` |
| 7 | Integration tests | `feat: integration tests` |
