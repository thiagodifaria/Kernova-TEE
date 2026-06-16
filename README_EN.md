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

## Documentation

**Main README:** [README.md](README.md)  
**Portuguese README:** [README_PT.md](README_PT.md)  
**Architecture:** [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)

---

## Overview

Kernova-TEE models a trusted execution boundary built around hardware-assisted virtualization on x86-64. The project keeps the Intel VMX path, adds an AMD SVM path for AMD hosts and introduces a Linux kernel driver so privileged validation can move from userspace modeling to `/dev/kernova`. The goal is to explore how a hardware-controlled monitor can isolate sensitive memory, intercept integrity-relevant events and move data through controlled marshalling paths.

The project is intentionally close to the hardware. Most VMX and SIMD operations are implemented in x86-64 Assembly, while C++20 provides the orchestration layer for state management, lifecycle control, testing and development-mode execution.

Kernova-TEE is best understood as a low-level security research and systems programming project. It is not a production TEE implementation and does not claim to replace mature hypervisors, hardware TEEs or kernel security frameworks.

---

## Problem Statement

Traditional endpoint security runs at the same privilege level as the kernel or above ordinary user processes but below hardware virtualization control. Once malicious code operates in Ring 0, it can modify syscall entry points, patch kernel code, alter descriptor tables, hide memory regions and interfere with integrity checks performed by the operating system itself.

Kernova explores a different control plane:

- The monitor runs at VMX root mode, commonly referred to as Ring -1.
- The operating system is moved into VMX non-root mode.
- Sensitive monitor and enclave regions are kept outside the guest's normal control.
- Writes to selected registers, MSRs and descriptor-related state can be routed through VMExit handling.

The central design assumption is that a smaller, hardware-controlled monitor can observe and constrain selected OS-level behavior from a privilege level the guest kernel cannot directly override.

---

## Main Capabilities

- Type-1 micro-hypervisor structure targeting Ring -1 / VMX root operation.
- VMX bootstrap path with `VMXON`, VMCS preparation, `VMLAUNCH`, `VMRESUME`, `VMREAD`, `VMWRITE`, `VMXOFF` and `INVEPT`.
- VMCS guest/host state modeling and processor-based execution controls.
- VMExit-oriented monitoring for control-register writes, IA32_LSTAR changes, descriptor-table modifications and guest `VMCALL` operations.
- SIMD data marshalling routines using AVX2 and AVX-512-oriented Assembly.
- Secure copy, secure zeroing, pack/unpack and block-hash routines exposed as C-callable symbols.
- C++20 orchestration for virtualization backend selection, enclave buffers, monitor state and development-mode self-tests.
- Backend boundary prepared for Kernel Driver, Intel VMX, AMD SVM and userspace PoC fallback.
- Linux privileged validation driver exposing `/dev/kernova` through a shared ioctl ABI.
- AMD SVM Ring 0 preparation path with `EFER.SVME`, `VM_HSAVE_PA`, VMCB allocation, host-save area allocation and guarded `VMRUN`.
- Native test suite covering SIMD behavior, performance, VMCS field validation and integrity routines.
- Docker Compose development environment for reproducible Linux builds from Windows, macOS or Linux hosts.
- Flask validation console for local build, test, status and runtime workflows.

---

## Repository Layout

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
    include/
      vmx_defs.inc                VMCS encodings and VMX constants
      avx_macros.inc              SIMD macro library
      registers.hpp               CPUID, CR and MSR wrappers
      virtualization/             Backend interfaces and CPU feature model
    tests/                        Native CTest suites
    CMakeLists.txt                Primary build system
    Makefile                      Alternative low-level build path
    linker.ld                     Bare-metal memory layout

infra/
  Dockerfile                      Linux build and test image
  compose.yaml                    Builder, tester and web services

service-kernel/
  linux/
    kernova_main.c                /dev/kernova device and ioctl dispatcher
    amd_svm/                      AMD SVM privileged validation path
    intel_vmx/                    Intel VMX capability stub for this milestone
    include/                      Kernel-side driver state and interfaces
    Makefile                      Linux kernel module build entrypoint

shared/
  kernova_abi.h                   Shared user space/kernel ioctl contract

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

This layout follows the same repository convention used by the other service-oriented projects in the workspace: clients stay under `client-*`, backend implementations stay under `service-api/service-*`, privileged kernel components stay under `service-kernel`, infrastructure stays under `infra`, operational scripts stay under `scripts`, shared ABI files stay under `shared`, and architectural material stays under `docs`.

---

## Architecture

### Privilege Model

```text
Ring -1 / VMX root mode
  Kernova monitor
  VMX state management
  Enclave-style memory region
  VMExit handlers
  Integrity tracing

Ring 0 / VMX non-root or SVM guest mode
  Guest operating system
  Kernel drivers
  Syscall entry points
  Descriptor tables

Ring 3
  User applications
```

The monitor is designed to operate above the guest kernel. The guest continues running normally, but selected privileged events are modeled as VMExit surfaces that can be logged, inspected or constrained by the monitor.

### Initialization Flow

```text
_start (entry.asm)
  -> disable interrupts
  -> initialize stack and segment state
  -> check long mode
  -> check VMX support through CPUID and IA32_FEATURE_CONTROL
  -> prepare identity-mapped regions for VMX structures
  -> initialize SIMD state
  -> call C++ hypervisor initialization

hypervisor_init (main.cpp)
  -> initialize trace monitor
  -> allocate enclave buffer
  -> detect CPU vendor and virtualization capabilities
  -> select Kernel Driver, Intel VMX, AMD SVM or userspace PoC backend
  -> initialize hardware backend when available
  -> run development-mode self-tests
  -> enter proof-of-concept idle loop
```

### Virtualization Backend Boundary

`service-api/service-cpp/src/virtualization` owns backend selection and CPU capability detection. It preserves Intel VMX behind this boundary, prepares the AMD SVM runtime context, exposes structured validation data to the web console and prefers the Linux kernel driver when `/dev/kernova` is loaded.

| Backend | Current Status | Purpose |
|---------|----------------|---------|
| Kernel Driver | Privileged Linux path | Talks to `/dev/kernova` and delegates hardware validation to Ring 0 |
| Userspace PoC | Active fallback | Validates SIMD, enclave, trace and integrity flows without privileged instructions |
| Intel VMX | Existing target | Uses the current VMX/VMCS path when VT-x is available |
| AMD SVM | Minimal runtime context prepared | Detects AMD SVM features, prepares VMCB/host-save state and reserves `VMRUN` for the Linux kernel driver |

Hardware backend execution is disabled by default in development builds. To explicitly attempt hardware virtualization, set `KERNOVA_ENABLE_HARDWARE_BACKEND=1` or `KERNOVA_STRICT_VMX=1`.

### Linux Kernel Driver

`service-kernel/linux` is the privileged validation harness. It creates `/dev/kernova`, implements the shared ABI from `shared/kernova_abi.h` and starts with the AMD SVM path because the available bare-metal validation machine is AMD.

| ioctl | Purpose |
|-------|---------|
| `KERNOVA_IOCTL_QUERY_CAPS` | Report CPU vendor, VMX/SVM capability and driver state |
| `KERNOVA_IOCTL_INIT_BACKEND` | Initialize the privileged backend |
| `KERNOVA_IOCTL_CREATE_VM` | Allocate and prepare a minimal VM context |
| `KERNOVA_IOCTL_RUN_VM` | Attempt guest execution when explicitly enabled |
| `KERNOVA_IOCTL_DESTROY_VM` | Tear down allocated VM state |

### AMD SVM Backend Model

`service-api/service-cpp/src/virtualization/amd_svm` owns the AMD-V/SVM capability model used by backend selection. It detects SVM revision, ASID count and optional features such as nested paging, NRIP save, VMCB clean bits, decode assists and pause filtering. It also prepares a minimal runtime context with a 4KB-aligned VMCB page and host-save area. `VMRUN` remains blocked in userspace builds and is reserved for the privileged Linux driver.

| Area | Data / Interface | Purpose |
|------|------------------|---------|
| SVM discovery | CPUID `0x80000001`, `0x8000000A` | Detect SVM support and implementation features |
| Control MSRs | `EFER.SVME`, `VM_CR`, `VM_HSAVE_PA` | Model the registers required before `VMRUN` |
| VMCB page | `amd_svm::Vmcb` | Prepare the 4KB control/save-state page layout |
| Runtime context | `amd_svm::SvmRuntimeContext` | Hold VMCB, host-save area and launch state |
| Capability model | `amd_svm::SvmCapabilities` | Expose SVM readiness to backend selection and tests |

### Intel VMX Backend

`service-api/service-cpp/src/virtualization/intel_vmx/core` contains the low-level VMX routines.

| Operation | Instruction / Interface | Purpose |
|-----------|-------------------------|---------|
| Enable VMX | `CR4.VMXE` | Allow VMX operation on the processor |
| Enter VMX root operation | `VMXON` | Activate VMX root mode using a 4KB-aligned VMXON region |
| Load VMCS | `VMPTRLD` | Select the active Virtual Machine Control Structure |
| Clear VMCS | `VMCLEAR` | Reset VMCS launch state |
| Read VMCS | `VMREAD` | Inspect VMCS fields |
| Write VMCS | `VMWRITE` | Configure guest, host and control fields |
| Launch guest | `VMLAUNCH` | Enter VMX non-root guest execution |
| Resume guest | `VMRESUME` | Continue guest execution after VMExit |
| Leave VMX | `VMXOFF` | Deactivate VMX operation |
| Invalidate EPT translations | `INVEPT` | Flush extended-page-table translations |

Important MSRs:

| MSR | Address | Purpose |
|-----|---------|---------|
| `IA32_FEATURE_CONTROL` | `0x3A` | VMX enable and lock policy |
| `IA32_VMX_BASIC` | `0x480` | VMX revision ID and VMCS metadata |
| `IA32_VMX_PROCBASED_CTLS` | `0x482` | Processor-based VM-execution controls |
| `IA32_EFER` | `0xC0000080` | Long mode state and related execution features |

### SIMD Marshalling Engine

`service-api/service-cpp/src/marshall` implements high-throughput memory routines intended for host-to-enclave data movement.

| Function | Description |
|----------|-------------|
| `simd_pack_data` | Packs source data into a destination buffer with vectorized operations |
| `simd_unpack_data` | Extracts packed data back into a host-readable buffer |
| `simd_secure_memcpy` | Performs aligned vectorized copy |
| `simd_zero_memory` | Wipes memory using vector zeroing operations |
| `simd_hash_blocks` | Provides the current block-hash primitive used by tests |

Representative instruction families:

```nasm
VMOVDQA    ; aligned vector load/store
VPXOR      ; vector XOR and zeroing primitive
VZEROUPPER ; avoid AVX/SSE transition penalties
VMOVDQA64  ; AVX-512 aligned 64-byte vector access
VPXORQ     ; AVX-512 quadword XOR
```

### Hardware Trace Monitor

`service-api/service-cpp/src/monitor` models monitor-side event logging and integrity checks.

| Surface | Security relevance |
|---------|--------------------|
| CR0/CR3/CR4 writes | Paging, write-protection and VMX-related state can be altered here |
| IA32_LSTAR writes | Syscall entry-point hooks are commonly expressed through this MSR |
| IDTR/GDTR changes | Descriptor-table relocation can indicate interrupt or exception interception |
| `VMCALL` | Provides a guest-to-hypervisor request interface |
| Watchpoint configuration | Tracks memory regions for integrity-sensitive changes |

The C-callable monitor functions include `trace_init`, `trace_log_event`, `trace_check_integrity` and `trace_set_watchpoint`.

---

## Source Code Breakdown

| Module | Language | Purpose |
|--------|----------|---------|
| `src/virtualization/intel_vmx/boot/entry.asm` | NASM | Boot entry, long-mode checks, VMX support checks and SIMD setup |
| `src/virtualization/intel_vmx/core/vmx_init.asm` | NASM | VMXON region setup, VMX activation and VMX transition helpers |
| `src/virtualization/intel_vmx/core/vmcs_config.asm` | NASM | VMCS guest/host state and control-field configuration |
| `src/marshall/simd_packer.asm` | NASM | SIMD copy, zero, pack, unpack and block-hash routines |
| `src/virtualization/intel_vmx/monitor/intercept.asm` | NASM | VMExit dispatch and handler stubs |
| `src/monitor/trace_engine.asm` | NASM | Event logging, integrity checks and watchpoint handling |
| `src/main.cpp` | C++20 | Hypervisor, enclave, backend, monitor and test orchestration |
| `src/vmx_stubs.cpp` | C++20 | Development-mode VMX stubs for userspace builds |
| `src/virtualization/amd_svm/amd_svm.cpp` | C++20 | AMD SVM capability mapping and VMCB initialization |
| `src/virtualization/cpu_features.cpp` | C++20 | CPU vendor and VMX/SVM capability detection |
| `src/virtualization/hypervisor_backend.cpp` | C++20 | Backend selection for userspace PoC, Intel VMX and AMD SVM |
| `include/registers.hpp` | C++20 | CPUID, control-register and MSR helper wrappers |
| `include/virtualization/*.hpp` | C++20 | Backend interfaces and CPU feature model |
| `include/vmx_defs.inc` | NASM | VMCS constants and VMX definitions |
| `include/avx_macros.inc` | NASM | AVX helper macros |

---

## Requirements

### Host Requirements

| Requirement | Detail |
|-------------|--------|
| CPU | Intel VT-x or AMD-V/SVM processor for future hardware execution |
| Firmware | Virtualization enabled in BIOS/UEFI |
| SIMD | AVX2 recommended; AVX-512 paths are modeled where available |
| OS | Linux for direct native validation; Docker is recommended from Windows/macOS |
| Toolchain | CMake, NASM, GCC/G++, QEMU and Docker for the standard workflow |
| Kernel driver | Linux headers for the running kernel when building `service-kernel/linux` |

### Linux Packages

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake nasm qemu-system-x86 gdb python3 docker.io
```

---

## Build and Run

### Docker Build

```bash
docker compose -f infra/compose.yaml run --rm builder
```

### Docker Test

```bash
docker compose -f infra/compose.yaml run --rm tester
```

### Web Validation Console

```bash
docker compose -f infra/compose.yaml up web
```

The validation console is exposed at `http://localhost:5000`.

Console endpoints:

| Endpoint | Purpose |
|----------|---------|
| `/api/status` | Binary, dependency, system and virtualization status |
| `/api/validation` | Structured validation snapshot for the browser UI |
| `/api/test` | Runs the CTest suite |
| `/api/runtime` | Runs the Kernova runtime validation flow |

### Manual CMake Build

```bash
cmake -S service-api/service-cpp -B service-api/service-cpp/build -DCMAKE_BUILD_TYPE=Release
cmake --build service-api/service-cpp/build --parallel
ctest --test-dir service-api/service-cpp/build --output-on-failure
```

### Linux Kernel Driver

Build the module on Linux bare metal:

```bash
bash scripts/kernel-build.sh
```

Load the driver in safe mode:

```bash
bash scripts/kernel-load.sh
```

Load the driver with experimental AMD `VMRUN` enabled:

```bash
bash scripts/kernel-load.sh vmrun
KERNOVA_ENABLE_HARDWARE_BACKEND=1 ./service-api/service-cpp/build/Kernova
```

### QEMU

```bash
qemu-system-x86_64 -enable-kvm -m 2G -kernel service-api/service-cpp/build/Kernova -nographic
```

For debug mode:

```bash
qemu-system-x86_64 -enable-kvm -m 2G -kernel service-api/service-cpp/build/Kernova -s -S -nographic
gdb service-api/service-cpp/build/Kernova
```

---

## Testing

The project uses CTest from the C++ service build directory.

```bash
./scripts/test.sh
```

Test suites:

| Test | Scope | VT-x required |
|------|-------|---------------|
| `test_simd` | SIMD copy, zeroing, pack/unpack and hash behavior | No |
| `test_vmcs` | VMCS field encoding and configuration invariants | No direct VMX execution |
| `test_performance` | SIMD throughput and latency measurements | No |
| `test_integrity` | Hash determinism, avalanche behavior, collision checks and monitor routines | No |
| `test_backend` | CPU capability detection and virtualization backend selection | No |
| `test_amd_svm` | AMD SVM capability model and VMCB layout | No |

Current validation status:

- Docker/Linux build succeeds.
- `test_simd`, `test_vmcs`, `test_performance`, `test_integrity`, `test_backend` and `test_amd_svm` pass.
- Hardware VMX/SVM execution remains environment-gated and is not claimed by Docker validation.
- Linux kernel driver validation is pending bare-metal execution on a machine with matching kernel headers and virtualization enabled in firmware.

---

## Memory Layout

The linker script models the intended bare-metal memory organization:

```text
Address     Region                 Size    Purpose
0x100000    Hypervisor code         256KB   .text and read-only data
0x200000    VMXON region             4KB    VMXON structure with VMX revision ID
0x201000    VMCS region              4KB    Active Virtual Machine Control Structure
0x202000    Hypervisor stack         16KB   Ring -1 stack region
0x210000    Enclave region           1MB    Protected working memory
0x310000    Data buffer              64KB   SIMD staging buffer
0x410000    Trace buffer             64KB   Event and integrity trace storage
```

---

## Security Model

Kernova-TEE focuses on a narrow set of low-level threat surfaces.

| Threat surface | Detection or control path |
|----------------|---------------------------|
| Kernel rootkits modifying paging or control state | VMExit on selected control-register operations |
| Syscall table or syscall entry hooking | IA32_LSTAR monitoring |
| Descriptor-table manipulation | IDTR/GDTR integrity checks |
| Memory tampering in monitored regions | Watchpoints and integrity buffer checks |
| Guest-to-monitor communication | Restricted `VMCALL` interface |

Limitations:

- This is a proof of concept, not a formally verified TEE.
- The current hash primitive is not cryptographically sufficient, as reflected by `test_integrity`.
- VMX root execution cannot be validated inside standard Docker containers.
- AMD SVM `VMRUN` is present only through the Linux driver and is gated by `allow_vmrun=1` plus `KERNOVA_ENABLE_HARDWARE_BACKEND=1`.
- Hardware behavior depends on CPU model, firmware configuration and host virtualization policy.
- The project does not yet implement a complete EPT-based isolation policy suitable for production use.

---

## Benchmarks

The project includes benchmark-oriented tests for SIMD throughput and latency. Historical target figures documented by the project include:

| Operation | Target / observed class | Implementation approach |
|-----------|--------------------------|-------------------------|
| SIMD memcpy | Tens of GB/s on modern desktop CPUs | `VMOVDQA` / vectorized copy |
| Pack/unpack | High-throughput staging path | XOR and aligned vector load/store |
| Memory zeroing | Vector-width clearing | `VPXOR` plus vector stores |
| Block hashing | Experimental SIMD primitive | Assembly block routine |
| VMExit overhead | Hardware-dependent | VMX transition path |

Benchmark results should be interpreted as development measurements, not portable guarantees.

---

## Operational Notes

- Use Docker Compose for the most reliable build path from Windows.
- The Compose stack stores Linux build artifacts in a Docker volume to avoid conflicts with Windows CMake caches.
- Native Windows/MinGW builds may fail on POSIX allocation APIs and ELF-oriented assembly assumptions.
- QEMU execution requires a suitable kernel image and host virtualization support.
- Docker is appropriate for compilation, tests and the dashboard, but not for direct VT-x execution.

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).

## Contact

Thiago Di Faria - [thiagodifaria@gmail.com](mailto:thiagodifaria@gmail.com)
