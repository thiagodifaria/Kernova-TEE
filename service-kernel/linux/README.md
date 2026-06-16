# Kernova Linux Kernel Driver

This directory contains the privileged Linux validation driver for Kernova-TEE.
It creates `/dev/kernova` and exposes a small ioctl ABI shared with the C++
service through `shared/kernova_abi.h`.

## Scope

The driver currently implements the AMD SVM validation path:

- CPU capability discovery in Ring 0.
- AMD SVM initialization through `EFER.SVME` and `VM_HSAVE_PA`.
- 4KB VMCB allocation.
- Host-save area allocation.
- Minimal guest page allocation.
- Safe `RUN_VM` blocking by default.
- Experimental `VMRUN` path gated by `allow_vmrun=1` and `KERNOVA_ENABLE_HARDWARE_BACKEND=1`.

Intel VMX is detected but not executed by this first Linux driver milestone.

## Build

Install kernel headers for the running kernel, then run:

```bash
bash ../../scripts/kernel-build.sh
```

Or directly:

```bash
make
```

## Load In Safe Mode

Safe mode initializes the driver and allows `/dev/kernova` validation without
executing `VMRUN`:

```bash
bash ../../scripts/kernel-load.sh
```

Then run the web console or the C++ runtime. The selected backend should become
`Kernel Driver`.

## Experimental VMRUN Attempt

Only use this on bare metal when you are ready to test privileged execution:

```bash
bash ../../scripts/kernel-load.sh vmrun
KERNOVA_ENABLE_HARDWARE_BACKEND=1 ../../service-api/service-cpp/build/Kernova
```

This path is intentionally explicit because an invalid VMCB or unsupported
host state can crash or reset the machine.

## Unload

```bash
bash ../../scripts/kernel-unload.sh
```
