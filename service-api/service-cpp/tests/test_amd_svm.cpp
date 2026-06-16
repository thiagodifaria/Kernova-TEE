// =============================================================================
// AMD SVM Capability Model Test
// =============================================================================

#include <cstdio>
#include <cstdint>

#include "virtualization/amd_svm.hpp"

namespace virtualization = kernova::virtualization;
namespace amd_svm = kernova::virtualization::amd_svm;

int main() {
    auto features = virtualization::detect_cpu_features();
    auto capabilities = amd_svm::capabilities_from_cpu_features(features);

    std::printf("\n");
    std::printf("========================================\n");
    std::printf("  AMD SVM Capability Test\n");
    std::printf("========================================\n\n");

    std::printf("[*] CPU vendor: %s (%s)\n",
        virtualization::cpu_vendor_name(features.vendor),
        features.vendor_id);
    std::printf("[*] SVM supported: %s\n", capabilities.supported ? "yes" : "no");
    std::printf("[*] Revision: %u\n", capabilities.revision);
    std::printf("[*] ASIDs: %u\n", capabilities.asid_count);
    std::printf("[*] Nested paging: %s\n", capabilities.nested_paging ? "yes" : "no");
    std::printf("[*] NRIP save: %s\n", capabilities.nrip_save ? "yes" : "no");
    std::printf("[*] VMCB clean bits: %s\n", capabilities.vmcb_clean ? "yes" : "no");
    std::printf("[*] Summary: %s\n", amd_svm::capability_summary(capabilities));

    amd_svm::Vmcb vmcb{};
    vmcb.control_area[0] = 0xAA;
    vmcb.save_state_area[0] = 0xBB;
    amd_svm::initialize_vmcb(vmcb);

    const auto vmcb_address = reinterpret_cast<std::uintptr_t>(&vmcb);
    if ((vmcb_address % amd_svm::VMCB_SIZE) != 0) {
        std::printf("[-] VMCB is not 4KB aligned\n");
        return 1;
    }

    if (sizeof(vmcb) != amd_svm::VMCB_SIZE) {
        std::printf("[-] VMCB size is not 4KB\n");
        return 1;
    }

    if (vmcb.control_area[0] != 0 || vmcb.save_state_area[0] != 0) {
        std::printf("[-] VMCB initialization did not clear all state\n");
        return 1;
    }

    if (features.vendor == virtualization::CpuVendor::AMD && features.svm) {
        if (!capabilities.supported) {
            std::printf("[-] AMD SVM feature bit was detected but capability model is disabled\n");
            return 1;
        }
        if (!capabilities.has_minimum_runtime_support()) {
            std::printf("[-] AMD SVM detected but minimum runtime metadata is incomplete\n");
            return 1;
        }
    }

    amd_svm::SvmRuntimeContext context{};
    int prepare_result = amd_svm::prepare_runtime_context(context, features);

    if (capabilities.has_minimum_runtime_support()) {
        if (prepare_result != 0 || context.state != amd_svm::SvmRuntimeState::Prepared) {
            std::printf("[-] AMD SVM runtime context was not prepared\n");
            return 1;
        }

        int launch_result = amd_svm::attempt_vmrun(context);
        if (launch_result == 0 || context.state != amd_svm::SvmRuntimeState::LaunchBlocked) {
            std::printf("[-] VMRUN was not safely blocked in userspace test mode\n");
            return 1;
        }
    } else if (prepare_result == 0) {
        std::printf("[-] Runtime context prepared without minimum SVM support\n");
        return 1;
    }

    return 0;
}
