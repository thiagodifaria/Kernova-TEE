#ifndef KERNOVA_VIRTUALIZATION_AMD_SVM_HPP
#define KERNOVA_VIRTUALIZATION_AMD_SVM_HPP

#include "virtualization/cpu_features.hpp"

#include <cstddef>
#include <cstdint>

namespace kernova::virtualization::amd_svm {

constexpr uint32_t MSR_EFER = 0xC0000080;
constexpr uint32_t MSR_VM_CR = 0xC0010114;
constexpr uint32_t MSR_VM_HSAVE_PA = 0xC0010117;

constexpr uint64_t EFER_SVME = 1ULL << 12;
constexpr uint64_t VM_CR_SVMDIS = 1ULL << 4;

constexpr uint32_t CPUID_SVM_LEAF = 0x8000000A;
constexpr std::size_t VMCB_SIZE = 4096;
constexpr std::size_t VMCB_CONTROL_AREA_SIZE = 1024;
constexpr std::size_t VMCB_SAVE_STATE_SIZE = 1024;
constexpr std::size_t HOST_SAVE_AREA_SIZE = 4096;

enum class SvmRuntimeState {
    Unsupported,
    Supported,
    Prepared,
    LaunchBlocked,
    LaunchAttempted,
};

struct SvmCapabilities {
    bool supported{false};
    uint32_t revision{0};
    uint32_t asid_count{0};
    bool nested_paging{false};
    bool lbr_virtualization{false};
    bool svm_lock{false};
    bool nrip_save{false};
    bool tsc_rate_msr{false};
    bool vmcb_clean{false};
    bool flush_by_asid{false};
    bool decode_assists{false};
    bool pause_filter{false};
    bool pause_filter_threshold{false};
    bool avic{false};
    bool vmsave_virtualization{false};
    bool vgif{false};

    bool has_minimum_runtime_support() const noexcept {
        return supported && revision != 0 && asid_count != 0;
    }
};

struct alignas(4096) Vmcb {
    uint8_t control_area[VMCB_CONTROL_AREA_SIZE]{};
    uint8_t save_state_area[VMCB_SAVE_STATE_SIZE]{};
    uint8_t reserved[VMCB_SIZE - VMCB_CONTROL_AREA_SIZE - VMCB_SAVE_STATE_SIZE]{};
};

struct alignas(4096) HostSaveArea {
    uint8_t data[HOST_SAVE_AREA_SIZE]{};
};

struct SvmRuntimeContext {
    Vmcb vmcb{};
    HostSaveArea host_save_area{};
    SvmCapabilities capabilities{};
    SvmRuntimeState state{SvmRuntimeState::Unsupported};
    const char* last_error{"not initialized"};
};

static_assert(sizeof(Vmcb) == VMCB_SIZE, "AMD VMCB must occupy one 4KB page");
static_assert(alignof(Vmcb) == VMCB_SIZE, "AMD VMCB must be 4KB-aligned");
static_assert(sizeof(HostSaveArea) == HOST_SAVE_AREA_SIZE, "AMD host save area must occupy one 4KB page");
static_assert(alignof(HostSaveArea) == HOST_SAVE_AREA_SIZE, "AMD host save area must be 4KB-aligned");

SvmCapabilities capabilities_from_cpu_features(const CpuFeatures& features) noexcept;
void initialize_vmcb(Vmcb& vmcb) noexcept;
void initialize_host_save_area(HostSaveArea& host_save_area) noexcept;
void prepare_minimal_vmcb(Vmcb& vmcb, const SvmCapabilities& capabilities) noexcept;
int prepare_runtime_context(SvmRuntimeContext& context, const CpuFeatures& features) noexcept;
int attempt_vmrun(SvmRuntimeContext& context) noexcept;
const char* capability_summary(const SvmCapabilities& capabilities) noexcept;
const char* runtime_state_name(SvmRuntimeState state) noexcept;

} // namespace kernova::virtualization::amd_svm

#endif // KERNOVA_VIRTUALIZATION_AMD_SVM_HPP
