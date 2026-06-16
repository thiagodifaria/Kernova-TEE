# Kernova-TEE

![Kernova-TEE](https://img.shields.io/badge/Kernova--TEE-Trusted%20Execution%20Environment-111827?style=for-the-badge)

**Kernova-TEE is a research-oriented Type-1 micro-hypervisor for x86-64 systems, implemented with NASM Assembly and a C++20 orchestration layer. It explores hardware-backed isolation, VMX root-mode monitoring, SIMD-assisted secure data movement and low-level integrity checks for trusted execution scenarios.**

[![Version](https://img.shields.io/badge/Version-1.0.0-2563EB?style=flat)](README.md)
[![Assembly](https://img.shields.io/badge/x86--64-NASM-525252?style=flat)](service-api/service-cpp/src)
[![C++](https://img.shields.io/badge/C++-20-00599C?style=flat&logo=cplusplus&logoColor=white)](service-api/service-cpp)
[![Intel VT-x](https://img.shields.io/badge/Intel%20VT--x-VMX-0071C5?style=flat&logo=intel&logoColor=white)](service-api/service-cpp/src/core)
[![SIMD](https://img.shields.io/badge/SIMD-AVX2%20%7C%20AVX--512-6B7280?style=flat)](service-api/service-cpp/src/marshall)
[![Runtime](https://img.shields.io/badge/Runtime-Docker%20Compose-2496ED?style=flat&logo=docker&logoColor=white)](infra)
[![License](https://img.shields.io/badge/License-MIT-success?style=flat)](LICENSE)

---

## Documentation / Documentacao

**Leia em portugues:** [README_PT.md](README_PT.md)  
**Read the detailed English README:** [README_EN.md](README_EN.md)  
**Architecture:** [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)

---

## What is Kernova-TEE?

Kernova-TEE is a compact hypervisor proof of concept designed around Intel VT-x. The project models a trusted execution boundary where the security monitor runs in VMX root mode while the operating system is treated as a guest in VMX non-root mode.

The implementation focuses on four technical areas:

- VMX bootstrap and VMCS configuration through hand-written x86-64 Assembly.
- A C++20 orchestration layer for initialization, enclave state, monitor state and test execution.
- SIMD marshalling routines for high-throughput data movement and memory operations.
- VMExit-oriented monitoring hooks for control-register, MSR and integrity-sensitive events.

This repository is not organized as a conventional application backend. The native hypervisor code is treated as a language-specific service under `service-api/service-cpp`, with CLI and web clients separated from the low-level backend and Docker assets isolated under `infra`.

---

## Main Capabilities

- Type-1 micro-hypervisor structure targeting Ring -1 / VMX root mode.
- VMX initialization flow with `VMXON`, VMCS setup, `VMLAUNCH`, `VMRESUME`, `VMREAD`, `VMWRITE`, `VMXOFF` and `INVEPT` routines.
- VMCS guest/host state modeling and processor-control configuration.
- Hardware-oriented monitoring for CR0/CR3/CR4 writes, IA32_LSTAR modification, descriptor-table changes and guest `VMCALL` requests.
- SIMD pack, unpack, secure copy, secure zeroing and block-hash routines using AVX2 and AVX-512-oriented code paths.
- C++20 state model for hypervisor, enclave, VMX and monitor orchestration.
- Native test suite for SIMD behavior, VMCS encoding, performance measurements and integrity routines.
- Docker-based Linux build environment for reproducible native compilation from Windows, macOS or Linux hosts.
- CLI and Flask dashboard for local build, test, status and QEMU workflows.

---

## Repository Layout

The project follows the same service-oriented layout used across LexiconCLI, MoodAPI and SchemaAPI:

```text
client-cli/
  main.py                         Terminal management client

client-web/
  app.py                          Flask dashboard for build/test/status operations
  requirements.txt

service-api/
  service-cpp/
    src/
      boot/                       Boot entry, long-mode checks and processor setup
      core/                       VMX initialization and VMCS configuration
      marshall/                   SIMD pack/unpack/copy/hash routines
      monitor/                    VMExit interception and integrity tracing
      main.cpp                    C++20 orchestration layer
      vmx_stubs.cpp               Userspace-safe VMX stubs for development builds
    include/                      VMX definitions, AVX macros and register wrappers
    tests/                        Native CTest suites
    CMakeLists.txt                Primary build system
    Makefile                      Alternative low-level build path
    linker.ld                     Bare-metal memory layout

infra/
  Dockerfile                      Linux build and test image
  compose.yaml                    Builder, tester and web services

scripts/
  build.sh                        Build, clean, package and pipeline entrypoint
  test.sh                         CMake + CTest entrypoint
  docker-entrypoint.sh            Container command dispatcher

docs/
  ARCHITECTURE.md                 Repository and subsystem boundaries
```

---

## Architecture Summary

Kernova is divided into three core subsystems.

### VMX Hypervisor Core

`service-api/service-cpp/src/core` owns the VMX activation path and VMCS configuration. It is responsible for enabling VMX operation, preparing VMXON and VMCS regions, loading the active VMCS and modeling transitions between VMX root and non-root operation.

Key instructions and interfaces include:

| Area | Instructions / Data | Purpose |
|------|---------------------|---------|
| VMX activation | `VMXON`, `VMXOFF` | Enter and leave VMX root operation |
| VMCS control | `VMPTRLD`, `VMCLEAR`, `VMREAD`, `VMWRITE` | Manage the active Virtual Machine Control Structure |
| Guest execution | `VMLAUNCH`, `VMRESUME` | Transfer execution to the guest and resume after VMExit |
| TLB / EPT maintenance | `INVEPT` | Invalidate extended-page-table translations |
| VMX metadata | `IA32_VMX_BASIC`, `IA32_FEATURE_CONTROL` | Discover revision IDs and enable VMX outside SMX |

### SIMD Marshalling Engine

`service-api/service-cpp/src/marshall` implements fast data movement between host-controlled buffers and enclave-style memory regions. The routines are written in Assembly and expose C-callable symbols used by the C++ layer and tests.

| Function | Role |
|----------|------|
| `simd_pack_data` | Pack data into a protected buffer using vector operations |
| `simd_unpack_data` | Copy data out of the protected buffer |
| `simd_secure_memcpy` | Aligned vectorized memory copy |
| `simd_zero_memory` | Secure memory wipe using vector zeroing |
| `simd_hash_blocks` | SIMD-oriented block hashing primitive |

### Hardware Trace Monitor

`service-api/service-cpp/src/monitor` models integrity checks and event tracing for VMExit-driven security monitoring.

| Event Surface | Intended Detection |
|---------------|--------------------|
| CR0/CR3/CR4 access | Control-register tampering and paging changes |
| IA32_LSTAR writes | Syscall entry-point hooking |
| IDTR/GDTR changes | Descriptor-table manipulation |
| `VMCALL` | Guest-to-hypervisor requests |
| Integrity buffers | Memory modification checks and watchpoints |

---

## Quick Start

Build with Docker, which is the recommended path on Windows hosts:

```bash
docker compose -f infra/compose.yaml run --rm builder
```

Run the native test suite:

```bash
docker compose -f infra/compose.yaml run --rm tester
```

Run the CLI:

```bash
python3 client-cli/main.py
```

Run the web dashboard:

```bash
docker compose -f infra/compose.yaml up web
```

Manual Linux build:

```bash
cmake -S service-api/service-cpp -B service-api/service-cpp/build -DCMAKE_BUILD_TYPE=Release
cmake --build service-api/service-cpp/build --parallel
ctest --test-dir service-api/service-cpp/build --output-on-failure
```

---

## Validation Status

The Docker/Linux builder compiles the C++/NASM service successfully.

The current CTest suite builds all targets and runs four test groups:

| Test | Scope | Current status |
|------|-------|----------------|
| `test_simd` | SIMD copy, pack/unpack, zeroing and hash behavior | Passing |
| `test_vmcs` | VMCS field encoding and configuration invariants | Passing |
| `test_performance` | SIMD throughput and latency checks | Passing |
| `test_integrity` | Hash avalanche, collision behavior and monitor checks | Failing known hash-quality assertions |

The `test_integrity` failure is not caused by the repository reorganization. It indicates that the current `simd_hash_blocks` primitive does not yet provide the avalanche and collision properties expected by the test suite.

---

## Operating Notes

- VT-x execution requires compatible Intel hardware with virtualization enabled in firmware.
- A regular Docker container cannot provide direct VMX root-mode execution. Docker is used for build, test and dashboard workflows.
- Native Windows/MinGW builds may fail on POSIX/C allocation APIs and ELF-oriented assembly assumptions. The supported validation path is Linux or the Docker Compose environment.
- QEMU execution is intended for development and proof-of-concept validation, not production deployment.

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).

## Contact

Thiago Di Faria - [thiagodifaria@gmail.com](mailto:thiagodifaria@gmail.com)
