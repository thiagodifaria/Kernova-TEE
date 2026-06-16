#include "virtualization/amd_svm.hpp"

namespace kernova::virtualization::amd_svm {

SvmCapabilities capabilities_from_cpu_features(const CpuFeatures& features) noexcept {
    SvmCapabilities capabilities{};
    capabilities.supported = features.vendor == CpuVendor::AMD && features.svm;
    capabilities.revision = features.svm_revision;
    capabilities.asid_count = features.svm_asid_count;
    capabilities.nested_paging = features.svm_nested_paging;
    capabilities.lbr_virtualization = features.svm_lbr_virtualization;
    capabilities.svm_lock = features.svm_lock;
    capabilities.nrip_save = features.svm_nrip_save;
    capabilities.tsc_rate_msr = features.svm_tsc_rate_msr;
    capabilities.vmcb_clean = features.svm_vmcb_clean;
    capabilities.flush_by_asid = features.svm_flush_by_asid;
    capabilities.decode_assists = features.svm_decode_assists;
    capabilities.pause_filter = features.svm_pause_filter;
    capabilities.pause_filter_threshold = features.svm_pause_filter_threshold;
    capabilities.avic = features.svm_avic;
    capabilities.vmsave_virtualization = features.svm_vmsave_virtualization;
    capabilities.vgif = features.svm_vgif;
    return capabilities;
}

void initialize_vmcb(Vmcb& vmcb) noexcept {
    vmcb = Vmcb{};
}

void initialize_host_save_area(HostSaveArea& host_save_area) noexcept {
    host_save_area = HostSaveArea{};
}

void prepare_minimal_vmcb(Vmcb& vmcb, const SvmCapabilities& capabilities) noexcept {
    initialize_vmcb(vmcb);

    // Control-area offsets follow AMD's VMCB page model. Phase 4 prepares the
    // fields the hardware path will consume; it does not execute VMRUN by default.
    auto write16 = [&vmcb](std::size_t offset, uint16_t value) {
        vmcb.control_area[offset + 0] = static_cast<uint8_t>(value & 0xFF);
        vmcb.control_area[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    };

    auto write32 = [&vmcb](std::size_t offset, uint32_t value) {
        vmcb.control_area[offset + 0] = static_cast<uint8_t>(value & 0xFF);
        vmcb.control_area[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
        vmcb.control_area[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
        vmcb.control_area[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
    };

    write16(0x000, 0xFFFF); // intercept CR reads
    write16(0x002, 0xFFFF); // intercept CR writes
    write32(0x00C, 1U << 0); // intercept INTR as a conservative default

    if (capabilities.nested_paging) {
        write32(0x090, 1U); // enable nested paging bit in NP_ENABLE field
    }

    if (capabilities.vmcb_clean) {
        write32(0x0C0, 0); // VMCB clean bits: force full hardware reload first
    }
}

int prepare_runtime_context(SvmRuntimeContext& context, const CpuFeatures& features) noexcept {
    context = SvmRuntimeContext{};
    context.capabilities = capabilities_from_cpu_features(features);

    if (!context.capabilities.has_minimum_runtime_support()) {
        context.state = SvmRuntimeState::Unsupported;
        context.last_error = capability_summary(context.capabilities);
        return -1;
    }

    initialize_host_save_area(context.host_save_area);
    prepare_minimal_vmcb(context.vmcb, context.capabilities);
    context.state = SvmRuntimeState::Prepared;
    context.last_error = "AMD SVM runtime context prepared; VMRUN requires privileged execution";
    return 0;
}

int attempt_vmrun(SvmRuntimeContext& context) noexcept {
    if (context.state != SvmRuntimeState::Prepared) {
        context.last_error = "AMD SVM context is not prepared";
        return -1;
    }

    context.state = SvmRuntimeState::LaunchBlocked;
    context.last_error = "VMRUN is blocked in userspace builds; use the Linux kernel driver for privileged execution";
    return -2;
}

const char* capability_summary(const SvmCapabilities& capabilities) noexcept {
    if (!capabilities.supported) {
        return "AMD SVM is not advertised by CPUID";
    }

    if (!capabilities.has_minimum_runtime_support()) {
        return "AMD SVM is present, but minimum runtime metadata is incomplete";
    }

    if (capabilities.nested_paging && capabilities.nrip_save) {
        return "AMD SVM is present with nested paging and NRIP save";
    }

    return "AMD SVM is present with baseline VMRUN support";
}

const char* runtime_state_name(SvmRuntimeState state) noexcept {
    switch (state) {
        case SvmRuntimeState::Unsupported:
            return "unsupported";
        case SvmRuntimeState::Supported:
            return "supported";
        case SvmRuntimeState::Prepared:
            return "prepared";
        case SvmRuntimeState::LaunchBlocked:
            return "launch-blocked";
        case SvmRuntimeState::LaunchAttempted:
            return "launch-attempted";
        default:
            return "unknown";
    }
}

} // namespace kernova::virtualization::amd_svm
