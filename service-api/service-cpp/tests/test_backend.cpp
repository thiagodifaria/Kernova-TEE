// =============================================================================
// Virtualization Backend Selection Test
// =============================================================================

#include <cstdio>
#include <cstring>

#include "virtualization/hypervisor_backend.hpp"

namespace virtualization = kernova::virtualization;

int main() {
    auto features = virtualization::detect_cpu_features();
    auto& backend = virtualization::select_backend();
    auto status = backend.status();

    std::printf("\n");
    std::printf("========================================\n");
    std::printf("  Virtualization Backend Test\n");
    std::printf("========================================\n\n");

    std::printf("[*] CPU vendor: %s (%s)\n",
        virtualization::cpu_vendor_name(features.vendor),
        features.vendor_id);
    std::printf("[*] VMX: %s\n", features.vmx ? "yes" : "no");
    std::printf("[*] SVM: %s\n", features.svm ? "yes" : "no");
    std::printf("[*] Selected backend: %s\n", backend.name());
    std::printf("[*] Backend mode: %s\n", status.mode);
    std::printf("[*] Backend reason: %s\n", status.reason);

    if (std::strlen(features.vendor_id) == 0) {
        std::printf("[-] CPU vendor id is empty\n");
        return 1;
    }

    if (status.cpu.vendor != features.vendor) {
        std::printf("[-] Backend status vendor does not match detected vendor\n");
        return 1;
    }

    if (backend.kind() == virtualization::BackendKind::KernelDriver) {
        return status.kernel_driver_available && status.hardware_supported ? 0 : 1;
    }

    if (features.vendor == virtualization::CpuVendor::Intel && features.vmx) {
        return backend.kind() == virtualization::BackendKind::IntelVMX ? 0 : 1;
    }

    if (features.vendor == virtualization::CpuVendor::AMD && features.svm) {
        return backend.kind() == virtualization::BackendKind::AmdSVM ? 0 : 1;
    }

    return backend.kind() == virtualization::BackendKind::UserspacePoC ? 0 : 1;
}
