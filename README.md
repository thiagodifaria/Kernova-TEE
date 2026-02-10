# Kernova-TEE

![Kernova](https://img.shields.io/badge/Kernova--TEE-Trusted%20Execution%20Environment-0d9488?style=for-the-badge)

**Type-1 Micro-Hypervisor in x86-64 Assembly for Hardware-Level Security Isolation**

[![Assembly](https://img.shields.io/badge/x86--64-Assembly%20(NASM)-blue?style=flat&logo=assemblyscript&logoColor=white)]()
[![C++20](https://img.shields.io/badge/C++-20-00599C?style=flat&logo=cplusplus&logoColor=white)]()
[![Intel VT-x](https://img.shields.io/badge/Intel-VT--x%20%7C%20VMX-0071C5?style=flat&logo=intel&logoColor=white)]()
[![AVX-512](https://img.shields.io/badge/SIMD-AVX2%20%7C%20AVX--512-orange?style=flat)]()
[![License](https://img.shields.io/badge/License-MIT-green.svg?style=flat)](LICENSE)

---

## 🌍 **Documentation / Documentação**

**📖 [🇺🇸 Read in English](README_EN.md)**
**📖 [🇧🇷 Leia em Português](README_PT.md)**

---

## 🎯 What is Kernova?

Kernova-TEE is a **Type-1 micro-hypervisor** that creates a hardware-isolated Trusted Execution Environment using Intel VT-x virtualization. Written in raw x86-64 Assembly with a C++20 orchestration layer, it operates at **Ring -1** — above the OS kernel — to provide absolute security isolation.

### ⚡ Key Highlights

- 🛡️ **Hardware Isolation** — Ring -1 privilege via Intel VT-x (VMX root mode)
- ⚡ **Zero-Latency SIMD** — AVX2/AVX-512 data transport @ 45 GB/s
- 🔍 **Rootkit Detection** — Hardware-trace on LSTAR, IDTR, CR writes
- 🔐 **Cryptographic Enclave** — 1MB isolated memory region
- 🧬 **Bare-Metal Assembly** — ~1,630 lines of hand-crafted x86-64 ASM
- 🔬 **C++20 Orchestrator** — Enclave, VMX, and monitoring namespaces
- 🐳 **Docker Support** — Containerized build, test, and web interface

### 🏆 What Makes It Special?

```
✅ Real VMX instructions (VMXON, VMLAUNCH, VMRESUME, INVEPT)
✅ VMCS configuration from Intel SDM specification
✅ SIMD marshalling engine (AVX2 + AVX-512)
✅ VMExit-based rootkit detection
✅ VMCALL guest-to-hypervisor interface
✅ Custom linker script with memory isolation
✅ Multiboot2 bootable via QEMU
```

---

## ⚡ Quick Start

```bash
git clone https://github.com/thiagodifaria/Kernova.git
cd Kernova

# Option 1: Interactive CLI (recommended)
python3 interface/cli.py

# Option 2: CMake
mkdir build && cd build
cmake .. && make -j$(nproc)
ctest --verbose

# Option 3: Docker
docker-compose -f docker/docker-compose.yml run builder
```

---

## 🏗️ Architecture

```
           Ring -1 (VMX Root Mode)
┌─────────────────────────────────────┐
│         KERNOVA-TEE CORE            │
│  ┌──────────┐  ┌─────────────────┐  │
│  │ Boot     │  │ VMX Init        │  │
│  │ entry.asm│→ │ VMXON → VMCS    │  │
│  └──────────┘  └─────────────────┘  │
│  ┌──────────────────────────────┐   │
│  │ SIMD Marshalling Engine      │   │
│  │ AVX2/AVX-512 @ 45 GB/s       │   │
│  └──────────────────────────────┘   │
│  ┌──────────────────────────────┐   │
│  │ Hardware Trace Monitor       │   │
│  │ VMExit Intercept + Rootkit   │   │
│  └──────────────────────────────┘   │
├─────────────────────────────────────┤
│    Ring 0 — Guest OS (Demoted)      │
└─────────────────────────────────────┘
```

---

## 📁 Project Structure

```
Kernova/
├── src/           # x86-64 Assembly + C++20 (~1,630 lines)
├── include/       # VMX defs, AVX macros, register wrappers
├── tests/         # 4 test suites (SIMD, perf, integrity, VMCS)
├── interface/     # Python CLI + Flask web dashboard
├── scripts/       # Build and test automation
├── docker/        # Dockerfile + docker-compose.yml
├── CMakeLists.txt # Primary build system
├── Makefile       # Alternative build
└── linker.ld      # Custom memory layout
```

---

## 📞 Contact

**Thiago Di Faria** — [thiagodifaria@gmail.com](mailto:thiagodifaria@gmail.com)

[![GitHub](https://img.shields.io/badge/GitHub-@thiagodifaria-black?style=flat&logo=github)](https://github.com/thiagodifaria)

---

### 🌟 **Star this project if you're into low-level security!**

**Made with ❤️ by [Thiago Di Faria](https://github.com/thiagodifaria)**

*Isolation. Performance. Vigilance.* 🛡️
