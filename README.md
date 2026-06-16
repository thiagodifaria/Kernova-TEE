# Kernova-TEE

![Kernova-TEE](https://img.shields.io/badge/Kernova--TEE-Trusted%20Execution%20Environment-111827?style=for-the-badge)

**Kernova-TEE is a research-oriented x86-64 trusted execution project implemented with NASM Assembly, C++20 and a privileged Linux validation driver. It explores hardware-backed isolation through Intel VMX and AMD SVM, SIMD-assisted secure data movement and low-level integrity checks for trusted execution scenarios.**

[![Version](https://img.shields.io/badge/Version-2.0.0-2563EB?style=flat)](README.md)
[![Assembly](https://img.shields.io/badge/x86--64-NASM-525252?style=flat)](service-api/service-cpp/src)
[![C++](https://img.shields.io/badge/C++-20-00599C?style=flat&logo=cplusplus&logoColor=white)](service-api/service-cpp)
[![Intel VT-x](https://img.shields.io/badge/Intel%20VT--x-VMX-0071C5?style=flat&logo=intel&logoColor=white)](service-api/service-cpp/src/virtualization/intel_vmx/core)
[![AMD SVM](https://img.shields.io/badge/AMD--V-SVM-ED1C24?style=flat)](service-api/service-cpp/src/virtualization/amd_svm)
[![Kernel Driver](https://img.shields.io/badge/Linux%20Driver-/dev/kernova-111827?style=flat&logo=linux&logoColor=white)](service-kernel/linux)
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

Kernova-TEE is a compact hypervisor proof of concept designed around hardware-assisted virtualization on x86-64. The project models a trusted execution boundary where the security monitor targets VMX root mode on Intel systems and AMD SVM on AMD systems, with privileged Linux driver validation available through `/dev/kernova`.

The implementation focuses on four technical areas:

- VMX bootstrap and VMCS configuration through hand-written x86-64 Assembly.
- A C++20 orchestration layer for initialization, enclave state, monitor state and test execution.
- SIMD marshalling routines for high-throughput data movement and memory operations.
- VMExit-oriented monitoring hooks for control-register, MSR and integrity-sensitive events.

This repository is not organized as a conventional application backend. The native hypervisor code is treated as a language-specific service under `service-api/service-cpp`, with the web validation console separated from the low-level backend and Docker assets isolated under `infra`.

---

## Main Capabilities

- Type-1 micro-hypervisor structure targeting Ring -1 / VMX root mode.
- VMX initialization flow with `VMXON`, VMCS setup, `VMLAUNCH`, `VMRESUME`, `VMREAD`, `VMWRITE`, `VMXOFF` and `INVEPT` routines.
- VMCS guest/host state modeling and processor-control configuration.
- Hardware-oriented monitoring for CR0/CR3/CR4 writes, IA32_LSTAR modification, descriptor-table changes and guest `VMCALL` requests.
- SIMD pack, unpack, secure copy, secure zeroing and block-hash routines using AVX2 and AVX-512-oriented code paths.
- C++20 state model for hypervisor, enclave, virtualization backend and monitor orchestration.
- Virtualization backend boundary prepared for Kernel Driver, Intel VMX, AMD SVM and userspace PoC fallback.
- Linux privileged validation driver exposing `/dev/kernova` through a shared ioctl ABI.
- AMD SVM Ring 0 preparation path with `EFER.SVME`, `VM_HSAVE_PA`, VMCB allocation, host-save area allocation and guarded `VMRUN`.
- Native test suite for SIMD behavior, VMCS encoding, performance measurements and integrity routines.
- Docker-based Linux build environment for reproducible native compilation from Windows, macOS or Linux hosts.
- Flask validation console for local build, test, status and runtime checks.

---

## Repository Layout

The project follows the same service-oriented layout used across LexiconCLI, MoodAPI and SchemaAPI:

```text
client-web/
  app.py                          Flask validation console for build/test/status operations
  requirements.txt

service-api/
  service-cpp/
    src/
      marshall/                   SIMD pack/unpack/copy/hash routines
      monitor/                    Trace engine and integrity routines
      virtualization/
        amd_svm/                  AMD SVM capability and VMCB model
        intel_vmx/                Intel VMX boot, VMCS and VMExit routines
        cpu_features.cpp          CPU vendor and capability detection
        hypervisor_backend.cpp    Backend selection and lifecycle
      main.cpp                    C++20 orchestration layer
      vmx_stubs.cpp               Userspace-safe VMX stubs for development builds
    include/                      VMX definitions, AVX macros, register wrappers and backend interfaces
    tests/                        Native CTest suites
    CMakeLists.txt                Primary build system
    Makefile                      Alternative low-level build path
    linker.ld                     Bare-metal memory layout

service-kernel/
  linux/
    kernova_main.c                /dev/kernova device and ioctl dispatcher
    amd_svm/                      AMD SVM privileged validation path
    intel_vmx/                    Intel VMX capability stub for this milestone
    include/                      Kernel-side driver state and interfaces
    Makefile                      Linux kernel module build entrypoint

shared/
  kernova_abi.h                   Shared user space/kernel ioctl contract

infra/
  Dockerfile                      Linux build and test image
  compose.yaml                    Builder, tester and web services

scripts/
  build.sh                        Build, clean, package and pipeline entrypoint
  test.sh                         CMake + CTest entrypoint
  docker-entrypoint.sh            Container command dispatcher
  kernel-build.sh                 Linux kernel module build helper
  kernel-load.sh                  Safe or VMRUN-enabled driver load helper
  kernel-unload.sh                Driver unload helper

docs/
  ARCHITECTURE.md                 Repository and subsystem boundaries
```

---

## Architecture Summary

Kernova is divided into the following core subsystems.

### Kernel Driver Boundary

`service-kernel/linux` owns privileged hardware validation. It creates `/dev/kernova` and exposes the ioctl ABI defined in `shared/kernova_abi.h`. The C++ service detects this device first; when it is loaded, backend selection moves from the userspace SVM/VMX model to the `Kernel Driver` backend.

| Command | Purpose |
|---------|---------|
| `KERNOVA_IOCTL_QUERY_CAPS` | Report CPU vendor, VMX/SVM capability and driver state |
| `KERNOVA_IOCTL_INIT_BACKEND` | Initialize the privileged backend, starting with AMD SVM |
| `KERNOVA_IOCTL_CREATE_VM` | Allocate and prepare a minimal VM context |
| `KERNOVA_IOCTL_RUN_VM` | Attempt guest execution when explicitly enabled |
| `KERNOVA_IOCTL_DESTROY_VM` | Tear down allocated VM state |

### Intel VMX Backend

`service-api/service-cpp/src/virtualization/intel_vmx/core` owns the VMX activation path and VMCS configuration. It is responsible for enabling VMX operation, preparing VMXON and VMCS regions, loading the active VMCS and modeling transitions between VMX root and non-root operation.

Key instructions and interfaces include:

| Area | Instructions / Data | Purpose |
|------|---------------------|---------|
| VMX activation | `VMXON`, `VMXOFF` | Enter and leave VMX root operation |
| VMCS control | `VMPTRLD`, `VMCLEAR`, `VMREAD`, `VMWRITE` | Manage the active Virtual Machine Control Structure |
| Guest execution | `VMLAUNCH`, `VMRESUME` | Transfer execution to the guest and resume after VMExit |
| TLB / EPT maintenance | `INVEPT` | Invalidate extended-page-table translations |
| VMX metadata | `IA32_VMX_BASIC`, `IA32_FEATURE_CONTROL` | Discover revision IDs and enable VMX outside SMX |

### AMD SVM Backend Model

`service-api/service-cpp/src/virtualization/amd_svm` owns the AMD-V/SVM capability model used by backend selection. It detects SVM revision, ASID count and optional features such as nested paging, NRIP save, VMCB clean bits, decode assists and pause filtering. It also prepares a minimal runtime context with a 4KB-aligned VMCB page and host-save area. `VMRUN` remains blocked in userspace builds and is reserved for the privileged Linux driver.

| Area | Data / Interface | Purpose |
|------|------------------|---------|
| SVM discovery | CPUID `0x80000001`, `0x8000000A` | Detect SVM support and implementation features |
| Control MSRs | `EFER.SVME`, `VM_CR`, `VM_HSAVE_PA` | Model the registers required before `VMRUN` |
| VMCB page | `amd_svm::Vmcb` | Prepare the 4KB control/save-state page layout |
| Runtime context | `amd_svm::SvmRuntimeContext` | Hold VMCB, host-save area and launch state |
| Capability model | `amd_svm::SvmCapabilities` | Expose SVM readiness to backend selection and tests |

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

Run the web validation console:

```bash
docker compose -f infra/compose.yaml up web
```

The console exposes:

| Endpoint | Purpose |
|----------|---------|
| `/api/status` | Binary, dependency, system and virtualization status |
| `/api/validation` | Structured validation snapshot for the browser UI |
| `/api/test` | Runs the CTest suite |
| `/api/runtime` | Runs the Kernova runtime validation flow |

Manual Linux build:

```bash
cmake -S service-api/service-cpp -B service-api/service-cpp/build -DCMAKE_BUILD_TYPE=Release
cmake --build service-api/service-cpp/build --parallel
ctest --test-dir service-api/service-cpp/build --output-on-failure
```

Linux bare-metal driver build:

```bash
bash scripts/kernel-build.sh
```

Load the driver in safe mode:

```bash
bash scripts/kernel-load.sh
```

Load the driver with the experimental AMD `VMRUN` path enabled:

```bash
bash scripts/kernel-load.sh vmrun
KERNOVA_ENABLE_HARDWARE_BACKEND=1 ./service-api/service-cpp/build/Kernova
```

---

## Validation Status

The Docker/Linux builder compiles the C++/NASM service successfully.

The current CTest suite builds all targets and runs six test groups:

| Test | Scope | Current status |
|------|-------|----------------|
| `test_simd` | SIMD copy, pack/unpack, zeroing and hash behavior | Passing |
| `test_vmcs` | VMCS field encoding and configuration invariants | Passing |
| `test_performance` | SIMD throughput and latency checks | Passing |
| `test_integrity` | Hash avalanche, collision behavior and monitor checks | Passing |
| `test_backend` | CPU capability detection and backend selection | Passing |
| `test_amd_svm` | AMD SVM capability model and VMCB layout | Passing |

Hardware VMX/SVM execution is still environment-gated. Docker validates build, test, backend selection and userspace PoC runtime, but not Ring -1 execution.

The Linux kernel driver is the hardware-validation path. In safe mode it validates Ring 0 capability detection and AMD SVM setup without entering the guest. The `VMRUN` attempt is intentionally gated by both `allow_vmrun=1` at module load time and `KERNOVA_ENABLE_HARDWARE_BACKEND=1` in the C++ runtime.

---

## Operating Notes

- Hardware virtualization execution requires compatible Intel VT-x or AMD-V/SVM hardware with virtualization enabled in firmware.
- Real privileged validation requires Linux bare metal and the `service-kernel/linux` driver.
- Development builds keep hardware backend execution disabled by default; use `KERNOVA_ENABLE_HARDWARE_BACKEND=1` or `KERNOVA_STRICT_VMX=1` to attempt it explicitly.
- A regular Docker container cannot provide direct VMX root-mode execution. Docker is used for build, test and dashboard workflows.
- Native Windows/MinGW builds may fail on POSIX/C allocation APIs and ELF-oriented assembly assumptions. The supported validation path is Linux or the Docker Compose environment.
- QEMU execution is intended for development and proof-of-concept validation, not production deployment.

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).

## Contact

Thiago Di Faria - [thiagodifaria@gmail.com](mailto:thiagodifaria@gmail.com)
