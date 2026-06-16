#include "virtualization/cpu_features.hpp"

#include <cstring>

namespace kernova::virtualization {
namespace {

void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx) noexcept {
    asm volatile("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(leaf), "c"(subleaf)
    );
}

} // namespace

CpuFeatures detect_cpu_features() noexcept {
    CpuFeatures features{};
    uint32_t eax{}, ebx{}, ecx{}, edx{};

    cpuid(0, 0, eax, ebx, ecx, edx);
    std::memcpy(features.vendor_id + 0, &ebx, sizeof(ebx));
    std::memcpy(features.vendor_id + 4, &edx, sizeof(edx));
    std::memcpy(features.vendor_id + 8, &ecx, sizeof(ecx));
    features.vendor_id[12] = '\0';

    if (std::strcmp(features.vendor_id, "GenuineIntel") == 0) {
        features.vendor = CpuVendor::Intel;
    } else if (std::strcmp(features.vendor_id, "AuthenticAMD") == 0) {
        features.vendor = CpuVendor::AMD;
    }

    cpuid(1, 0, eax, ebx, ecx, edx);
    features.vmx = ((ecx >> 5) & 1U) != 0;

    cpuid(7, 0, eax, ebx, ecx, edx);
    features.avx2 = ((ebx >> 5) & 1U) != 0;
    features.avx512 = ((ebx >> 16) & 1U) != 0;

    cpuid(0x80000000U, 0, eax, ebx, ecx, edx);
    if (eax >= 0x80000001U) {
        cpuid(0x80000001U, 0, eax, ebx, ecx, edx);
        features.svm = ((ecx >> 2) & 1U) != 0;
    }

    cpuid(0x80000000U, 0, eax, ebx, ecx, edx);
    if (features.svm && eax >= 0x8000000AU) {
        cpuid(0x8000000AU, 0, eax, ebx, ecx, edx);
        features.svm_revision = eax & 0xFFU;
        features.svm_asid_count = ebx;
        features.svm_nested_paging = ((edx >> 0) & 1U) != 0;
        features.svm_lbr_virtualization = ((edx >> 1) & 1U) != 0;
        features.svm_lock = ((edx >> 2) & 1U) != 0;
        features.svm_nrip_save = ((edx >> 3) & 1U) != 0;
        features.svm_tsc_rate_msr = ((edx >> 4) & 1U) != 0;
        features.svm_vmcb_clean = ((edx >> 5) & 1U) != 0;
        features.svm_flush_by_asid = ((edx >> 6) & 1U) != 0;
        features.svm_decode_assists = ((edx >> 7) & 1U) != 0;
        features.svm_pause_filter = ((edx >> 10) & 1U) != 0;
        features.svm_pause_filter_threshold = ((edx >> 12) & 1U) != 0;
        features.svm_avic = ((edx >> 13) & 1U) != 0;
        features.svm_vmsave_virtualization = ((edx >> 15) & 1U) != 0;
        features.svm_vgif = ((edx >> 16) & 1U) != 0;
    }

    return features;
}

const char* cpu_vendor_name(CpuVendor vendor) noexcept {
    switch (vendor) {
        case CpuVendor::Intel:
            return "Intel";
        case CpuVendor::AMD:
            return "AMD";
        default:
            return "Unknown";
    }
}

} // namespace kernova::virtualization
