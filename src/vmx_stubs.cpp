// =============================================================================
// Kernova-TEE - VMX Stub Implementations (Development/Testing Only)
// =============================================================================
// These stubs replace the bare-metal VMX assembly functions when building
// for userspace (development/testing). The real implementations live in
// kernova_asm_boot (entry.asm, vmx_init.asm, vmcs_config.asm, intercept.asm)
// and require Ring -1 (VMX root) privileges.
// =============================================================================

#include <cstdint>
#include <cstdio>

extern "C" {

void exception_handler() {
    std::printf("[STUB] exception_handler called\n");
}

int64_t vmx_init() {
    std::printf("[STUB] vmx_init - VMX not available in userspace\n");
    return 0; // Simulate success
}

void vmxon() {
    std::printf("[STUB] vmxon - VMX not available in userspace\n");
}

void vmxoff() {
    std::printf("[STUB] vmxoff - VMX not available in userspace\n");
}

int64_t vmx_launch() {
    std::printf("[STUB] vmx_launch - VMX not available in userspace\n");
    return 0;
}

int64_t vmx_resume() {
    std::printf("[STUB] vmx_resume - VMX not available in userspace\n");
    return 0;
}

int64_t vmcs_setup() {
    std::printf("[STUB] vmcs_setup - VMCS not available in userspace\n");
    return 0;
}

int64_t vmcs_clear(uint64_t) {
    std::printf("[STUB] vmcs_clear - VMCS not available in userspace\n");
    return 0;
}

int64_t vmcs_load(uint64_t) {
    std::printf("[STUB] vmcs_load - VMCS not available in userspace\n");
    return 0;
}

int64_t vmcs_launch_state() {
    std::printf("[STUB] vmcs_launch_state - VMCS not available in userspace\n");
    return 0;
}

} // extern "C"
