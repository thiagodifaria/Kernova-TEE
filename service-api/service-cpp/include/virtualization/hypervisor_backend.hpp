#ifndef KERNOVA_VIRTUALIZATION_HYPERVISOR_BACKEND_HPP
#define KERNOVA_VIRTUALIZATION_HYPERVISOR_BACKEND_HPP

#include "virtualization/cpu_features.hpp"

namespace kernova::virtualization {

enum class BackendKind {
    UserspacePoC,
    KernelDriver,
    IntelVMX,
    AmdSVM,
};

struct BackendStatus {
    BackendKind kind{BackendKind::UserspacePoC};
    CpuFeatures cpu{};
    bool hardware_supported{false};
    bool hardware_validated{false};
    bool kernel_driver_available{false};
    const char* mode{"userspace-poc"};
    const char* reason{"hardware virtualization backend not selected"};
};

class HypervisorBackend {
public:
    virtual ~HypervisorBackend() = default;

    virtual const char* name() const noexcept = 0;
    virtual BackendKind kind() const noexcept = 0;
    virtual BackendStatus status() const noexcept = 0;
    virtual int init() noexcept = 0;
    virtual int setup_guest() noexcept = 0;
    virtual int launch() noexcept = 0;
    virtual void shutdown() noexcept = 0;
};

HypervisorBackend& select_backend() noexcept;
const char* backend_kind_name(BackendKind kind) noexcept;

} // namespace kernova::virtualization

#endif // KERNOVA_VIRTUALIZATION_HYPERVISOR_BACKEND_HPP
