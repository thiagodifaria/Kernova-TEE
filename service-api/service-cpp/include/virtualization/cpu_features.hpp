#ifndef KERNOVA_VIRTUALIZATION_CPU_FEATURES_HPP
#define KERNOVA_VIRTUALIZATION_CPU_FEATURES_HPP

#include <cstdint>

namespace kernova::virtualization {

enum class CpuVendor {
    Intel,
    AMD,
    Unknown,
};

struct CpuFeatures {
    CpuVendor vendor{CpuVendor::Unknown};
    char vendor_id[13]{};
    bool vmx{false};
    bool svm{false};
    bool avx2{false};
    bool avx512{false};
    uint32_t svm_revision{0};
    uint32_t svm_asid_count{0};
    bool svm_nested_paging{false};
    bool svm_lbr_virtualization{false};
    bool svm_lock{false};
    bool svm_nrip_save{false};
    bool svm_tsc_rate_msr{false};
    bool svm_vmcb_clean{false};
    bool svm_flush_by_asid{false};
    bool svm_decode_assists{false};
    bool svm_pause_filter{false};
    bool svm_pause_filter_threshold{false};
    bool svm_avic{false};
    bool svm_vmsave_virtualization{false};
    bool svm_vgif{false};
};

CpuFeatures detect_cpu_features() noexcept;
const char* cpu_vendor_name(CpuVendor vendor) noexcept;

} // namespace kernova::virtualization

#endif // KERNOVA_VIRTUALIZATION_CPU_FEATURES_HPP
