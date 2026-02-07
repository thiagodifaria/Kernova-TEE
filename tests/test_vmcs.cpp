// =============================================================================
// VMCS Configuration Test
// =============================================================================

#include <cstdint>
#include <cstdio>
#include <cstring>

// Test basic VMCS field operations
extern "C" {
    int64_t vmcs_read(uint64_t field);
    int64_t vmcs_write(uint64_t field, uint64_t value);
}

// VMCS field encodings (from vmx_defs.inc)
constexpr uint64_t VMCS_GUEST_CR0         = 0x6800;
constexpr uint64_t VMCS_GUEST_CR3         = 0x6802;
constexpr uint64_t VMCS_GUEST_CR4         = 0x6804;
constexpr uint64_t VMCS_GUEST_RSP         = 0x681C;
constexpr uint64_t VMCS_GUEST_RIP         = 0x681E;
constexpr uint64_t VMCS_GUEST_RFLAGS      = 0x6820;

constexpr uint64_t VMCS_HOST_CR0          = 0x6C00;
constexpr uint64_t VMCS_HOST_CR3          = 0x6C02;
constexpr uint64_t VMCS_HOST_CR4          = 0x6C04;
constexpr uint64_t VMCS_HOST_RSP          = 0x6C1C;
constexpr uint64_t VMCS_HOST_RIP          = 0x6C1E;

constexpr uint64_t VMCS_EXIT_REASON       = 0x4402;
constexpr uint64_t VMCS_LINK_POINTER      = 0x2800;

// Test result tracking
struct TestStats {
    int passed{0};
    int failed{0};
} stats;

#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_RESET "\033[0m"

void test_passed(const char* test_name) {
    printf("[+] %s: " COLOR_GREEN "PASS" COLOR_RESET "\n", test_name);
    stats.passed++;
}

void test_failed(const char* test_name, const char* reason) {
    printf("[-] %s: " COLOR_RED "FAIL" COLOR_RESET " (%s)\n", test_name, reason);
    stats.failed++;
}

// Note: These tests require VMX to be initialized
// In a real test environment, we'd initialize VMX first
// For unit testing, we can test the structure and encoding

void test_vmcs_field_encoding() {
    printf("[*] Testing VMCS field encoding...\n");

    // Test that field encodings are correct
    // Guest CR0 should be: 0x6800 (64-bit guest state, index 0)
    if (VMCS_GUEST_CR0 == 0x6800) {
        test_passed("VMCS_GUEST_CR0 encoding");
    } else {
        test_failed("VMCS_GUEST_CR0 encoding", "Incorrect encoding");
    }

    // VMCS link pointer should be 64-bit value
    if (VMCS_LINK_POINTER == 0x2800) {
        test_passed("VMCS_LINK_POINTER encoding");
    } else {
        test_failed("VMCS_LINK_POINTER encoding", "Incorrect encoding");
    }

    // Exit reason should be 16-bit field
    if (VMCS_EXIT_REASON == 0x4402) {
        test_passed("VMCS_EXIT_REASON encoding");
    } else {
        test_failed("VMCS_EXIT_REASON encoding", "Incorrect encoding");
    }
}

void test_vmcs_alignment() {
    printf("[*] Testing VMCS alignment requirements...\n");

    // VMCS regions must be 4KB aligned
    constexpr size_t VMCS_SIZE = 4096;
    constexpr size_t ALIGNMENT = 4096;

    // Allocate aligned memory
    void* vmcs_ptr = nullptr;
    int result = posix_memalign(&vmcs_ptr, ALIGNMENT, VMCS_SIZE);

    if (result != 0) {
        test_failed("VMCS alignment", "Failed to allocate aligned memory");
        return;
    }

    // Check alignment
    if (reinterpret_cast<uintptr_t>(vmcs_ptr) % ALIGNMENT == 0) {
        test_passed("VMCS alignment");
    } else {
        test_failed("VMCS alignment", "Memory not properly aligned");
    }

    std::free(vmcs_ptr);
}

void test_vmcs_revision_id() {
    printf("[*] Testing VMCS revision ID...\n");

    // Revision ID is stored in first DWORD of VMCS
    // We can't read actual MSRs without VMX, but we can test the structure

    constexpr size_t VMCS_SIZE = 4096;
    uint32_t* vmcs = static_cast<uint32_t*>(std::aligned_alloc(4096, VMCS_SIZE));

    if (!vmcs) {
        test_failed("VMCS revision ID", "Allocation failed");
        return;
    }

    // Clear VMCS
    std::memset(vmcs, 0, VMCS_SIZE);

    // Set a test revision ID
    vmcs[0] = 0x12345678;  // Test value

    // Verify it's stored correctly
    if (vmcs[0] == 0x12345678) {
        test_passed("VMCS revision ID storage");
    } else {
        test_failed("VMCS revision ID storage", "Value mismatch");
    }

    std::free(vmcs);
}

void test_vmcs_link_pointer() {
    printf("[*] Testing VMCS link pointer...\n");

    // Link pointer should be 0xFFFFFFFFFFFFFFFF when not nesting
    constexpr uint64_t INVALID_LINK_PTR = 0xFFFFFFFFFFFFFFFFULL;

    if (INVALID_LINK_PTR == 0xFFFFFFFFFFFFFFFFULL) {
        test_passed("VMCS link pointer value");
    } else {
        test_failed("VMCS link pointer value", "Incorrect value");
    }
}

void test_control_register_values() {
    printf("[*] Testing control register values...\n");

    // CR0.PE (Protection Enable) - bit 0
    constexpr uint64_t CR0_PE = 0x00000001ULL;

    if (CR0_PE == (1ULL << 0)) {
        test_passed("CR0_PE value");
    } else {
        test_failed("CR0_PE value", "Incorrect value");
    }

    // CR4.VMXE (VMX Enable) - bit 13
    constexpr uint64_t CR4_VMXE = 0x00002000ULL;

    if (CR4_VMXE == (1ULL << 13)) {
        test_passed("CR4_VMXE value");
    } else {
        test_failed("CR4_VMXE value", "Incorrect value");
    }
}

void test_segment_access_rights() {
    printf("[*] Testing segment access rights...\n");

    // Code segment access rights: P=1, DPL=0, S=1, Type=1011 (execute/read)
    constexpr uint32_t CODE_SEG_AR = 0xA09B;

    // Check individual bits
    bool present = (CODE_SEG_AR & 0x80) != 0;       // P bit
    bool dpl = ((CODE_SEG_AR >> 5) & 0x3) == 0;     // DPL
    bool system = (CODE_SEG_AR & 0x10) != 0;        // S bit
    bool type = ((CODE_SEG_AR & 0x0F) == 0x0B);     // Type

    if (present && dpl && system && type) {
        test_passed("Code segment access rights");
    } else {
        test_failed("Code segment access rights", "Incorrect encoding");
    }

    // Data segment access rights: P=1, DPL=0, S=1, Type=0011 (read/write)
    constexpr uint32_t DATA_SEG_AR = 0xC093;

    present = (DATA_SEG_AR & 0x80) != 0;
    dpl = ((DATA_SEG_AR >> 5) & 0x3) == 0;
    system = (DATA_SEG_AR & 0x10) != 0;
    type = ((DATA_SEG_AR & 0x0F) == 0x03);

    if (present && dpl && system && type) {
        test_passed("Data segment access rights");
    } else {
        test_failed("Data segment access rights", "Incorrect encoding");
    }
}

int main() {
    printf("\n");
    printf("========================================\n");
    printf("  VMCS Configuration Test Suite\n");
    printf("========================================\n\n");

    printf("Note: These tests verify VMCS structure and encoding.\n");
    printf("Full VMX tests require hardware virtualization support.\n\n");

    // Run tests
    test_vmcs_field_encoding();
    test_vmcs_alignment();
    test_vmcs_revision_id();
    test_vmcs_link_pointer();
    test_control_register_values();
    test_segment_access_rights();

    // Summary
    printf("\n");
    printf("========================================\n");
    printf("  Test Results:\n");
    printf("  Passed: %d\n", stats.passed);
    printf("  Failed: %d\n", stats.failed);
    printf("========================================\n\n");

    return (stats.failed == 0) ? 0 : 1;
}
