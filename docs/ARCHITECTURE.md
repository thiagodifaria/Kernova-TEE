# Architecture

Kernova-TEE is organized by runtime responsibility. The repository root is an
orchestration layer, while each client, backend service and infrastructure
concern owns its implementation.

## Repository Boundaries

```text
client-cli/
client-web/
service-api/
  service-cpp/
infra/
scripts/
docs/
```

## C++ Service

`service-api/service-cpp` owns the hypervisor implementation and its native
build lifecycle.

- `src/boot`: boot entry and processor initialization.
- `src/core`: VT-x and VMCS configuration.
- `src/marshall`: SIMD data transport.
- `src/monitor`: VMExit interception and integrity tracing.
- `include`: C++ headers and NASM definitions.
- `tests`: native unit, integrity and performance tests.
- `CMakeLists.txt`, `Makefile` and `linker.ld`: service-specific build assets.

Build artifacts stay in `service-api/service-cpp/build` so the repository root
does not become coupled to one implementation language.

## Clients

`client-cli` provides the interactive terminal workflow for build, test,
status and QEMU commands.

`client-web` provides the Flask dashboard. It controls the local C++ service
but does not own native source code or build configuration.

## Infrastructure

`infra/Dockerfile` defines the common development image.
`infra/compose.yaml` exposes builder, tester and web services. Containers mount
the repository at `/workspace`, keep Linux build artifacts in a dedicated
Docker volume and invoke the root scripts or clients.

Hardware VT-x execution remains outside the regular container runtime.

## Scripts

Root scripts are stable entrypoints that resolve paths from their own location:

- `scripts/build.sh`: configure, build, clean and package.
- `scripts/test.sh`: configure, compile and run CTest.
- `scripts/docker-entrypoint.sh`: dispatch container commands.

This keeps path knowledge out of CI jobs and user commands while allowing the
C++ service to remain self-contained.
