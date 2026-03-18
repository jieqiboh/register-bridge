# FPGA Register Bridge — Scaffold Design
_Date: 2026-03-18_

## Goal
Build the initial scaffold for a minimal FPGA register interface simulator. Demonstrates register-level programming, binary protocol design, multi-threaded server architecture, and clean Python library API design.

## Decisions
- **Layout:** Flat (Option A) — `server/`, `client/`, `tests/` under one root
- **Register bank:** 256 registers, fixed, `uint32_t` values
- **Thread pool:** Hardcoded 4 threads
- **Scope:** Necessary requirements only (no AXI-Lite simulation, no DMA)

## Wire Protocol
7 bytes per packet: `[opcode: 1B | address: 2B | value: 4B]`

| Opcode | Hex  | Direction      |
|--------|------|----------------|
| READ   | 0x01 | client → server |
| WRITE  | 0x02 | client → server |
| ACK    | 0x03 | server → client |
| ERR    | 0x04 | server → client |

## Components

### C++ Server (`server/`)
- `protocol.hpp` — packet structs, opcode constants, pack/unpack helpers
- `register_bank.hpp/.cpp` — `std::array<uint32_t, 256>`, mutex-protected read/write
- `thread_pool.hpp/.cpp` — fixed 4-thread pool, task queue with condition variable
- `server.hpp/.cpp` — TCP accept loop, dispatches each client connection to thread pool
- `main.cpp` — parses `--port`, starts server

### Python Client (`client/quantumctrl/`)
- `protocol.py` — `pack_request()`, `unpack_response()` using `struct`
- `client.py` — `FPGAClient`: `connect()`, `disconnect()`, `read(addr)`, `write(addr, value)`

### Tests (`tests/`)
- `cpp/test_register_bank.cpp` — GTest: read/write, out-of-range, concurrent access
- `python/test_client.py` — pytest: all public methods with mocked socket
- `python/test_integration.py` — pytest: real server subprocess, end-to-end read/write

## Incremental Parts
1. Project skeleton (CMake, Python package stubs, empty test files)
2. Wire protocol — C++ structs + Python pack/unpack, no networking yet
3. Register bank — thread-safe array, unit tested
4. Thread pool — task queue + worker threads, unit tested
5. TCP server — accept loop + client handler, wired to register bank
6. Python client — socket wrapper, unit tested with mocks
7. Integration tests — end-to-end with real server
