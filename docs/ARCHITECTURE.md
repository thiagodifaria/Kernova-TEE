# Architecture

Kernova-TEE is organized by runtime responsibility. The repository root is an
orchestration layer, while each client, backend service and infrastructure
concern owns its implementation.

## Repository Boundaries

```text
client-web/
service-api/
  service-cpp/
service-kernel/
  linux/
shared/
infra/
scripts/
docs/
```

## C++ Service

`service-api/service-cpp` owns the hypervisor implementation and its native
build lifecycle.

- `src/marshall`: SIMD data transport.
- `src/virtualization`: CPU capability detection and backend selection.
- `src/virtualization/intel_vmx`: Intel VT-x boot, VMCS and VMExit routines.
- `src/virtualization/amd_svm`: AMD-V/SVM capability model, 4KB VMCB layout and minimal runtime context.
- `src/monitor`: trace engine and integrity routines.
- `include`: C++ headers and NASM definitions.
- `tests`: native unit, integrity and performance tests.
- `CMakeLists.txt`, `Makefile` and `linker.ld`: service-specific build assets.

Build artifacts stay in `service-api/service-cpp/build` so the repository root
does not become coupled to one implementation language.

## Shared ABI

`shared/kernova_abi.h` defines the stable user space/kernel contract used by
the C++ service and the Linux validation driver.

- `KERNOVA_IOCTL_QUERY_CAPS`: CPU vendor, VMX/SVM capability and driver state.
- `KERNOVA_IOCTL_INIT_BACKEND`: privileged backend initialization.
- `KERNOVA_IOCTL_CREATE_VM`: minimal VM context preparation.
- `KERNOVA_IOCTL_RUN_VM`: gated guest execution attempt.
- `KERNOVA_IOCTL_DESTROY_VM`: VM state teardown.

The ABI is intentionally small so the web console and C++ runtime can remain
stable while the privileged backend evolves.

## Kernel Service

`service-kernel/linux` owns the privileged validation path. It is not built by
Docker or the regular CMake workflow because it targets the running Linux
kernel on a bare-metal host.

- `kernova_main.c`: misc device registration, `/dev/kernova` and ioctl dispatch.
- `amd_svm`: AMD SVM Ring 0 setup, VMCB allocation, host-save area allocation
  and gated `VMRUN`.
- `intel_vmx`: Intel VMX capability stub for this milestone.
- `include`: kernel-side state and internal interfaces.
- `Makefile`: standard Linux kernel module build entrypoint.

Safe mode loads the driver and validates Ring 0 detection/setup without
entering the guest. Experimental execution requires both `allow_vmrun=1` when
loading the module and `KERNOVA_ENABLE_HARDWARE_BACKEND=1` in the C++ runtime.

## Clients

`client-web` provides the Flask validation console. It controls the local C++
service through scripts and process execution, exposes structured validation
snapshots through `/api/validation`, and does not own native source code or
build configuration.

The project intentionally no longer carries a terminal client. Operational
validation is centralized in the web console and the root scripts.

## Infrastructure

`infra/Dockerfile` defines the common development image.
`infra/compose.yaml` exposes builder, tester and web services. Containers mount
the repository at `/workspace`, keep Linux build artifacts in a dedicated
Docker volume and invoke the root scripts or clients.

Hardware VT-x/SVM execution remains outside the regular container runtime.
The container validates build, tests, backend selection and userspace runtime
preparation; privileged `VMLAUNCH`/`VMRUN` execution requires the Linux kernel
driver on bare metal.

## Scripts

Root scripts are stable entrypoints that resolve paths from their own location:

- `scripts/build.sh`: configure, build, clean and package.
- `scripts/test.sh`: configure, compile and run CTest.
- `scripts/docker-entrypoint.sh`: dispatch container commands.
- `scripts/kernel-build.sh`: build the Linux kernel module.
- `scripts/kernel-load.sh`: load the driver in safe mode or with `VMRUN`
  explicitly enabled.
- `scripts/kernel-unload.sh`: unload the driver and show recent kernel logs.

This keeps path knowledge out of CI jobs and user commands while allowing the
C++ service to remain self-contained.
