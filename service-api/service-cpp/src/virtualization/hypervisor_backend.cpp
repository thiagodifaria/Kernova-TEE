#include "virtualization/hypervisor_backend.hpp"
#include "virtualization/amd_svm.hpp"

#include <cstdlib>
#include <cstring>
#include <cstdio>

#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "kernova_abi.h"
#endif

extern "C" {
    int64_t vmx_init();
    void vmxoff();
    int64_t vmx_launch();
    int64_t vmcs_setup();
}

namespace kernova::virtualization {
namespace {

class UserspacePoCBackend final : public HypervisorBackend {
public:
    const char* name() const noexcept override { return "Userspace PoC"; }
    BackendKind kind() const noexcept override { return BackendKind::UserspacePoC; }

    BackendStatus status() const noexcept override {
        BackendStatus status{};
        status.kind = kind();
        status.cpu = detect_cpu_features();
        status.hardware_supported = false;
        status.hardware_validated = false;
        status.mode = "userspace-poc";
        status.reason = "hardware virtualization backend unavailable in this environment";
        return status;
    }

    int init() noexcept override {
        std::printf("[*] Using userspace PoC backend\n");
        return 0;
    }

    int setup_guest() noexcept override {
        std::printf("[*] Guest setup skipped in userspace PoC mode\n");
        return 0;
    }

    int launch() noexcept override {
        std::printf("[*] Guest launch skipped in userspace PoC mode\n");
        return 0;
    }

    void shutdown() noexcept override {}
};

bool hardware_backend_enabled() noexcept {
    const char* enabled = std::getenv("KERNOVA_ENABLE_HARDWARE_BACKEND");
    const char* strict = std::getenv("KERNOVA_STRICT_VMX");
    return (enabled && std::strcmp(enabled, "1") == 0) || (strict && std::strcmp(strict, "1") == 0);
}

#ifdef __linux__
class KernelDriverBackend final : public HypervisorBackend {
public:
    const char* name() const noexcept override { return "Kernel Driver"; }
    BackendKind kind() const noexcept override { return BackendKind::KernelDriver; }

    bool available() noexcept {
        if (fd_ >= 0) {
            return true;
        }

        fd_ = ::open(KERNOVA_DEVICE_PATH, O_RDWR | O_CLOEXEC);
        return fd_ >= 0;
    }

    BackendStatus status() const noexcept override {
        BackendStatus status{};
        status.kind = kind();
        status.cpu = detect_cpu_features();
        status.kernel_driver_available = fd_ >= 0;
        status.hardware_supported = false;
        status.hardware_validated = false;
        status.mode = "kernel-driver-unavailable";
        status.reason = "Kernova kernel driver is not loaded";

        if (fd_ < 0) {
            return status;
        }

        kernova_caps caps{};
        if (::ioctl(fd_, KERNOVA_IOCTL_QUERY_CAPS, &caps) != 0) {
            status.mode = "kernel-driver-error";
            status.reason = "Kernova kernel driver did not respond to QUERY_CAPS";
            return status;
        }

        status.kernel_driver_available = caps.driver_loaded != 0;
        status.hardware_supported = caps.svm_supported || caps.vmx_supported;
        status.hardware_validated = caps.last_vmexit_valid != 0;
        status.mode = caps.last_vmexit_valid
            ? "kernel-driver-vmexit"
            : (caps.vm_created ? "kernel-driver-vm-created" : "kernel-driver-ready");
        status.reason = caps.driver_name;
        return status;
    }

    int init() noexcept override {
        if (!available()) {
            std::printf("[!] Kernova kernel driver not loaded at %s\n", KERNOVA_DEVICE_PATH);
            return -1;
        }

        kernova_caps caps{};
        if (::ioctl(fd_, KERNOVA_IOCTL_QUERY_CAPS, &caps) != 0) {
            std::printf("[-] Failed to query Kernova kernel driver: errno=%d\n", errno);
            return -2;
        }

        kernova_init_request request{};
        if (caps.svm_supported) {
            request.backend_flags = KERNOVA_BACKEND_AMD_SVM;
        } else if (caps.vmx_supported) {
            request.backend_flags = KERNOVA_BACKEND_INTEL_VMX;
        } else {
            std::printf("[-] Kernova kernel driver found no VMX/SVM capability\n");
            return -3;
        }

        if (::ioctl(fd_, KERNOVA_IOCTL_INIT_BACKEND, &request) != 0) {
            std::printf("[-] Kernel backend initialization failed: errno=%d\n", errno);
            return -4;
        }

        initialized_ = true;
        std::printf("[+] Kernova kernel driver initialized\n");
        return 0;
    }

    int setup_guest() noexcept override {
        if (fd_ < 0 || !initialized_) {
            return -1;
        }

        kernova_caps caps{};
        if (::ioctl(fd_, KERNOVA_IOCTL_QUERY_CAPS, &caps) != 0) {
            return -2;
        }

        kernova_create_vm_request request{};
        request.backend_flags = caps.svm_supported ? KERNOVA_BACKEND_AMD_SVM : KERNOVA_BACKEND_INTEL_VMX;
        request.guest_size = 4096;

        if (::ioctl(fd_, KERNOVA_IOCTL_CREATE_VM, &request) != 0) {
            std::printf("[-] Kernel VM creation failed: errno=%d\n", errno);
            return -3;
        }

        configured_ = true;
        std::printf("[+] Kernel VM context prepared\n");
        return 0;
    }

    int launch() noexcept override {
        if (fd_ < 0 || !configured_) {
            return -1;
        }

        kernova_run_request request{};
        if (hardware_backend_enabled()) {
            request.flags = KERNOVA_RUN_ALLOW_PRIVILEGED;
        }

        if (::ioctl(fd_, KERNOVA_IOCTL_RUN_VM, &request) != 0) {
            std::printf("[!] Kernel VM launch blocked or failed: errno=%d state=%u status=%d\n",
                errno,
                request.runtime_state,
                request.status);
            return request.status ? request.status : -2;
        }

        std::printf("[+] Kernel VMEXIT received: code=%llu info1=%llu info2=%llu\n",
            static_cast<unsigned long long>(request.exit_code),
            static_cast<unsigned long long>(request.exit_info1),
            static_cast<unsigned long long>(request.exit_info2));
        return 0;
    }

    void shutdown() noexcept override {
        if (fd_ >= 0) {
            ::ioctl(fd_, KERNOVA_IOCTL_DESTROY_VM);
            ::close(fd_);
            fd_ = -1;
        }
        initialized_ = false;
        configured_ = false;
    }

private:
    mutable int fd_{-1};
    bool initialized_{false};
    bool configured_{false};
};
#endif

class IntelVmxBackend final : public HypervisorBackend {
public:
    const char* name() const noexcept override { return "Intel VMX"; }
    BackendKind kind() const noexcept override { return BackendKind::IntelVMX; }

    BackendStatus status() const noexcept override {
        BackendStatus status{};
        status.kind = kind();
        status.cpu = detect_cpu_features();
        status.hardware_supported = status.cpu.vendor == CpuVendor::Intel && status.cpu.vmx;
        status.hardware_validated = initialized_ && configured_;
        status.mode = status.hardware_validated ? "hardware-vmx" : "hardware-vmx-pending";
        status.reason = status.hardware_supported
            ? "Intel VMX is advertised by CPUID"
            : "Intel VMX is not available on this CPU/runtime";
        return status;
    }

    int init() noexcept override {
        auto current = status();
        if (!current.hardware_supported) {
            std::printf("[-] Intel VMX not supported by CPU/runtime\n");
            return -1;
        }

        if (!hardware_backend_enabled()) {
            std::printf("[!] Intel VMX support detected, but hardware backend execution is disabled by default\n");
            return -3;
        }

        std::printf("[*] Initializing Intel VMX backend...\n");
        int64_t result = vmx_init();
        if (result != 0) {
            std::printf("[-] Intel VMX initialization failed: %ld\n", result);
            return -2;
        }

        initialized_ = true;
        std::printf("[+] Intel VMX backend initialized\n");
        return 0;
    }

    int setup_guest() noexcept override {
        if (!initialized_) {
            return -1;
        }

        std::printf("[*] Setting up Intel VMCS...\n");
        int64_t result = vmcs_setup();
        if (result != 0) {
            std::printf("[-] Intel VMCS setup failed: %ld\n", result);
            return -2;
        }

        configured_ = true;
        std::printf("[+] Intel VMCS configured\n");
        return 0;
    }

    int launch() noexcept override {
        if (!configured_) {
            return -1;
        }

        return static_cast<int>(vmx_launch());
    }

    void shutdown() noexcept override {
        if (initialized_) {
            vmxoff();
            initialized_ = false;
            configured_ = false;
        }
    }

private:
    bool initialized_{false};
    bool configured_{false};
};

class AmdSvmBackend final : public HypervisorBackend {
public:
    const char* name() const noexcept override { return "AMD SVM"; }
    BackendKind kind() const noexcept override { return BackendKind::AmdSVM; }

    BackendStatus status() const noexcept override {
        BackendStatus status{};
        status.kind = kind();
        status.cpu = detect_cpu_features();
        auto capabilities = amd_svm::capabilities_from_cpu_features(status.cpu);
        status.hardware_supported = capabilities.has_minimum_runtime_support();
        status.hardware_validated = false;
        status.mode = amd_svm::runtime_state_name(context_.state);
        status.reason = context_.last_error ? context_.last_error : amd_svm::capability_summary(capabilities);
        return status;
    }

    int init() noexcept override {
        auto features = detect_cpu_features();
        int result = amd_svm::prepare_runtime_context(context_, features);
        auto capabilities = context_.capabilities;

        if (result != 0) {
            std::printf("[-] AMD SVM backend cannot be prepared: %s\n", context_.last_error);
            return result;
        }

        std::printf("[+] AMD SVM runtime context prepared\n");
        std::printf("[*] SVM revision: %u, ASIDs: %u, nested paging: %s, NRIP save: %s\n",
            capabilities.revision,
            capabilities.asid_count,
            capabilities.nested_paging ? "yes" : "no",
            capabilities.nrip_save ? "yes" : "no");
        return 0;
    }

    int setup_guest() noexcept override {
        if (context_.state != amd_svm::SvmRuntimeState::Prepared) {
            std::printf("[-] AMD SVM guest setup requires a prepared VMCB context\n");
            return -1;
        }

        std::printf("[+] AMD SVM minimal VMCB prepared for future VMRUN handoff\n");
        return 0;
    }

    int launch() noexcept override {
        int result = amd_svm::attempt_vmrun(context_);
        std::printf("[!] %s\n", context_.last_error);
        return result;
    }

    void shutdown() noexcept override {
        context_ = amd_svm::SvmRuntimeContext{};
    }

private:
    amd_svm::SvmRuntimeContext context_{};
};

UserspacePoCBackend userspace_backend;
#ifdef __linux__
KernelDriverBackend kernel_driver_backend;
#endif
IntelVmxBackend intel_backend;
AmdSvmBackend amd_backend;

} // namespace

HypervisorBackend& select_backend() noexcept {
    CpuFeatures features = detect_cpu_features();

#ifdef __linux__
    if (kernel_driver_backend.available()) {
        return kernel_driver_backend;
    }
#endif

    if (features.vendor == CpuVendor::Intel && features.vmx) {
        return intel_backend;
    }

    if (features.vendor == CpuVendor::AMD && features.svm) {
        return amd_backend;
    }

    return userspace_backend;
}

const char* backend_kind_name(BackendKind kind) noexcept {
    switch (kind) {
        case BackendKind::KernelDriver:
            return "Kernel Driver";
        case BackendKind::IntelVMX:
            return "Intel VMX";
        case BackendKind::AmdSVM:
            return "AMD SVM";
        case BackendKind::UserspacePoC:
        default:
            return "Userspace PoC";
    }
}

} // namespace kernova::virtualization
