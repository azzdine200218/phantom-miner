# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [2.1.0] - 2026-03-22

### Added
- Full Windows (MSVC) native build support via vcpkg toolchain
- `RemoteConfigClient` class for decoupled remote config fetching
- Explicit copy constructors for `MiningStats` to comply with MSVC `<atomic>` strict rules
- `GetSystemInfo()` fallback for page-size detection on Windows (replaces POSIX `sysconf`)
- `_access` / `F_OK` compatibility shim for Windows in `environment_detector.cpp`
- `reinterpret_cast` corrections in Winsock `send`/`recv` call sites

### Changed
- Migrated `nlohmann_json` from `FetchContent` to native vcpkg `find_package`
- Replaced POSIX `localtime_r` with `localtime_s` under `_WIN32` in `logger.cpp`
- Replaced `setsockopt` `timeval` struct with `DWORD` milliseconds on Windows in `pool_client.cpp`
- Unified `ProcessHider` namespace to `Stealth` across header and implementation
- Upgraded `randomx_alloc_cache` and `randomx_create_vm` calls with explicit `static_cast<randomx_flags>`
- Replaced hardcoded `RANDOMX_DATASET_MAX_SIZE` / `RANDOMX_CACHE_SIZE` constants with inline values for MSVC

### Fixed
- Namespace mismatch between `process_hider.h` (Stealth) and `process_hider.cpp` (XMR)
- `std::map::operator[]` called on a `const` reference in `miner.cpp` — replaced with `.at()`
- `MiningStats` copy-construction deleted by MSVC due to non-copyable `std::atomic` members
- ODR violation from two conflicting definitions of `RemoteConfig` (struct vs class)
- Missing `RANDOMX_DATASET_MAX_SIZE` identifier under Windows build

---

## [2.0.0] - 2025-11-10

### Added
- RandomX mining engine with NUMA awareness and HugePages support
- Pool balancer with automatic failover across multiple endpoints
- TLS/SSL pool connections via OpenSSL
- SOCKS5/Tor integration in `tor_handler.cpp`
- Telegram Bot C2 interface for real-time monitoring
- Remote configuration endpoint polling
- GPU miner stub (CUDA/OpenCL)
- Windows Defender bypass module (optional)
- Process injection framework (Linux)
- Sandbox and VM detection in `environment_detector.cpp`
- Persistent watchdog thread

### Changed
- Restructured codebase into distinct namespaces: `XMR`, `Stealth`, `Network`
- Switched build system to CMake 3.20 with modern target-based linking
- Replaced custom JSON parser with `nlohmann/json`

---

## [1.0.0] - 2024-08-01

### Added
- Initial release with basic Stratum pool client
- Single-threaded RandomX CPU miner
- Basic process renaming for stealth
