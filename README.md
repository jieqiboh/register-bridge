# register-bridge

A TCP server that exposes a bank of 256 hardware registers over a simple binary protocol, with a Python client library for reading and writing those registers.

The server is written in C++17 and handles concurrent clients via a fixed thread pool. The Python client (`quantumctrl`) mirrors the wire protocol and provides a minimal `read`/`write` API.

## What this simulates

This project simulates the register interface of an FPGA peripheral. In real hardware, a host CPU communicates with an FPGA over a bus (e.g. AXI-Lite or PCIe) by reading and writing a flat address space of memory-mapped registers. The FPGA exposes configuration, status, and data registers at fixed addresses; the driver issues byte-level read/write transactions to control it.

`register-bridge` replaces that hardware bus with a TCP connection, keeping the same register-address model and binary packet framing. The C++ server plays the role of the FPGA register file; the Python client plays the role of the host driver. This makes the project useful for:

- practising register-level programming and binary protocol design without real hardware
- testing driver code against a controlled, in-process register backend
- demonstrating multi-threaded server architecture and clean library API design

## Repository layout

```
server/          C++17 TCP server (register bank + thread pool)
client/          Python client library (quantumctrl package)
tests/cpp/       GoogleTest unit tests for the C++ components
tests/python/    pytest unit and integration tests
docs/plans/      Design documents
```

## Architecture

```
  Python Client (quantumctrl)
  ┌──────────────────────────────┐
  │ RegisterClient                   │
  │   read(addr) / write(addr,v) │
  │         │                    │
  │   protocol.py                │
  │   pack/unpack 7-byte packets │
  └──────────────┬───────────────┘
                 │ TCP (port 5555)
                 │ 7-byte packets
  ───────────────┼───────────────
  C++ Server (register_bridge)
  ┌──────────────┴───────────────┐
  │ Server                       │
  │   TCP accept loop            │
  │         │ enqueue            │
  │   ThreadPool (4 workers)     │
  │         │                    │
  │   handle_client(fd)          │
  │         │                    │
  │   RegisterBank               │
  │   256 × uint32, mutex-guarded│
  └──────────────────────────────┘
```

## Wire protocol

Every packet is exactly **7 bytes**, big-endian:

```
+--------+----------+----------+
| opcode | address  |  value   |
| 1 byte | 2 bytes  | 4 bytes  |
+--------+----------+----------+
```

| Opcode | Value | Direction      |
|--------|-------|----------------|
| READ   | 0x01  | client → server |
| WRITE  | 0x02  | client → server |
| ACK    | 0x03  | server → client |
| ERR    | 0x04  | server → client |

- **Read**: send `READ | addr | 0` → receive `ACK | addr | value`
- **Write**: send `WRITE | addr | value` → receive `ACK | addr | 0`

Valid addresses are `0x0000`–`0x00FF` (256 registers of `uint32`).

## Building the server

Requires CMake ≥ 3.16 and a C++17 compiler.

```sh
cmake -B build
cmake --build build
```

The server binary is at `build/server/register_bridge`.

## Running the server

```sh
./build/server/register_bridge             # default port 5555
./build/server/register_bridge --port 6000 # custom port
```

## Python client

Requires Python ≥ 3.11.

```sh
pip install -e client/
```

```python
from quantumctrl.client import RegisterClient

c = RegisterClient("localhost", 5555)
c.connect()
c.write(0x10, 0xDEADBEEF)
print(hex(c.read(0x10)))   # 0xdeadbeef
c.disconnect()
```

## Running the tests

**C++ unit tests**

```sh
cmake --build build
ctest --test-dir build --output-on-failure
```

**Python unit tests**

```sh
pytest tests/python/test_protocol.py tests/python/test_client.py -v
```

**Integration tests** (requires the server binary to be built first)

```sh
SERVER_BINARY=./build/server/register_bridge pytest tests/python/test_integration.py -v
```
