# phantom-miner

> High-performance, modular Monero (XMR) mining engine written in modern C++17.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-blue)](#)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](#)
[![Build](https://img.shields.io/badge/build-CMake%203.20%2B-green)](#)

---

## Overview

`phantom-miner` is a cross-platform Monero CPU/GPU miner engineered for reliability, performance, and modularity. It is built on top of the [RandomX](https://github.com/tevador/RandomX) proof-of-work algorithm and supports multi-pool failover, TLS-encrypted Stratum connections, and optional Tor routing for privacy.

---

## Features

- **RandomX Engine** — Full RandomX implementation with AVX2/AVX512 auto-detection, HugePages, and NUMA-aware memory allocation
- **Multi-pool Failover** — Automatic reconnection and pool rotation on connection failure
- **TLS/SSL Support** — Encrypted Stratum connections via OpenSSL
- **Tor Routing** — Optional SOCKS5 proxy integration for anonymized pool connections
- **Process Stealth** — Watchdog thread, process renaming, and environment detection
- **GPU Mining** — Optional CUDA/OpenCL acceleration (requires CUDA Toolkit 10+)
- **Remote Config** — Periodic config refresh from a remote endpoint
- **Telegram C2** — Real-time stats and remote management via Telegram Bot API
- **Cross-platform** — Native support for Windows (MSVC + vcpkg) and Linux (GCC/Clang)

---

## Requirements

### All Platforms
| Dependency | Version |
|---|---|
| C++ Compiler | MSVC 2019+, GCC 8+, or Clang 7+ |
| CMake | 3.20+ |
| OpenSSL | 1.1.1+ |
| libcurl | 7.60+ |
| nlohmann/json | 3.x (via vcpkg) |

### Optional
| Dependency | Purpose |
|---|---|
| CUDA Toolkit 10+ | NVIDIA GPU acceleration |
| libnuma-dev | NUMA-aware memory (Linux only) |

---

## Build

### Windows (MSVC + vcpkg)

```powershell
# Clone and bootstrap vcpkg (once)
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat

# Install dependencies
C:\vcpkg\vcpkg install curl:x64-windows openssl:x64-windows nlohmann-json:x64-windows

# Configure and build
cmake -B build -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake `
      -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The output binary is at `build\Release\phantom_miner.exe`.

### Linux (GCC/Clang)

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt install build-essential cmake libssl-dev libcurl4-openssl-dev nlohmann-json3-dev

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

The output binary is at `build/phantom_miner`.

---

## Configuration

Copy `config.json` to the same directory as the binary and edit before running.

### Minimal `config.json` example

```json
{
  "mining": {
    "max_threads": 4,
    "max_cpu_usage": 80,
    "randomx": { "enabled": true, "huge_pages": false }
  },
  "network": {
    "wallet": "YOUR_XMR_WALLET_ADDRESS",
    "password": "x",
    "pools": [
      { "url": "pool.supportxmr.com:3333", "tls": false }
    ]
  },
  "stealth": {
    "hide_process": false,
    "watchdog": { "enabled": false }
  }
}
```

### Config Sections

| Section | Description |
|---|---|
| `mining` | Thread count, CPU usage cap, RandomX options |
| `network` | Wallet, passwords, pool list, Tor settings |
| `stealth` | Process hiding, watchdog, sandbox detection |
| `c2` | Telegram bot integration for remote stats |
| `remote_config` | URL for fetching live config updates |

---

## Project Structure

```
phantom-miner/
├── include/               # Public headers
│   ├── miner/             # Mining engine interfaces
│   ├── network/           # Pool and network interfaces
│   ├── stealth/           # Stealth module interfaces
│   ├── gpu/               # GPU miner interface
│   ├── c2/                # C2 module interfaces
│   └── utils/             # Config, logger, crypto
├── src/                   # Implementation files
│   ├── miner/
│   ├── network/
│   ├── stealth/
│   ├── gpu/
│   ├── c2/
│   ├── utils/
│   └── main.cpp
├── third_party/           # RandomX submodule
├── tests/                 # Unit tests
├── scripts/               # Helper scripts
├── config.json            # Default configuration
└── CMakeLists.txt
```

---

## Premium Edition

Looking for the **advanced version** with extended features, priority support, and custom configurations?

Reach out for more details:

| Channel | Contact |
|---|---|
| **Telegram** | [@Go_gf](https://t.me/Go_gf) |
| **Email** | [azrazr351@gmail.com](mailto:azrazr351@gmail.com) |

### Custom Development

Need a **custom-built tool** tailored to your specific requirements? I develop advanced, production-grade software solutions on demand. Whether it's a specialized engine, a private toolkit, or a fully bespoke system — let's discuss what we can build together.

> I'm always open to ambitious projects and innovative collaborations. Don't hesitate to reach out.

---

## License

Released under the [MIT License](LICENSE).

---

*This software is provided for educational and research purposes only.*
