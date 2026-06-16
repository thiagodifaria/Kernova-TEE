// =============================================================================
// Kernova-TEE - C++ Orchestration Layer
// =============================================================================
// High-level management and initialization for the hypervisor system
// =============================================================================

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <utility>

#include "virtualization/hypervisor_backend.hpp"

// =============================================================================
// External Assembly Functions
// =============================================================================
extern "C" {
    // SIMD marshalling
    int64_t simd_pack_data(uint8_t* dest, const uint8_t* src, uint64_t size, uint64_t flags);
    int64_t simd_unpack_data(uint8_t* dest, const uint8_t* src, uint64_t size, uint64_t flags);
    int64_t simd_secure_memcpy(uint8_t* dest, const uint8_t* src, uint64_t size);
    int64_t simd_zero_memory(uint8_t* buffer, uint64_t size);
    int64_t simd_hash_blocks(const uint8_t* data, uint64_t size, uint8_t* hash_out);

    // Monitoring and tracing
    int64_t trace_init();
    int64_t trace_log_event(uint64_t event_type, uint64_t data);
    int64_t trace_check_integrity();
    int64_t trace_set_watchpoint(uint64_t address, uint64_t size, uint64_t type);

}

// =============================================================================
// Constants and Configuration
// =============================================================================
namespace config {
    constexpr uint64_t ENCLAVE_SIZE = 0x100000;        // 1MB secure enclave
    constexpr uint64_t DATA_BUFFER_SIZE = 0x10000;     // 64KB data buffer
    constexpr uint64_t NUM_TRACES = 1024;
    constexpr uint64_t HEARTBEAT_INTERVAL_MS = 1000;

    bool strict_vmx_required() {
        const char* value = std::getenv("KERNOVA_STRICT_VMX");
        return value && std::strcmp(value, "1") == 0;
    }

    bool hardware_backend_enabled() {
        const char* value = std::getenv("KERNOVA_ENABLE_HARDWARE_BACKEND");
        return value && std::strcmp(value, "1") == 0;
    }
}

// =============================================================================
// Hypervisor State
// =============================================================================
struct HypervisorState {
    bool hardware_virtualization_validated{false};
    bool trace_enabled{false};
    bool enclave_active{false};

    uint64_t enclave_base{0};

    // Statistics
    uint64_t vmexits_total{0};
    uint64_t vmexits_per_second{0};
    uint64_t data_transferred_bytes{0};

    // Security events
    uint64_t suspicious_events{0};
    uint64_t integrity_violations{0};
};

static HypervisorState g_hv_state;
static kernova::virtualization::HypervisorBackend* g_backend{nullptr};

// =============================================================================
// Enclave Management
// =============================================================================
namespace enclave {

struct EnclaveBuffer {
    uint8_t* data{nullptr};
    uint64_t size{0};

    bool allocate(uint64_t sz) {
        // Allocate aligned memory for enclave
        // In production, would use proper page allocation
        data = static_cast<uint8_t*>(std::aligned_alloc(4096, sz));
        if (!data) return false;

        size = sz;
        std::memset(data, 0, sz);
        return true;
    }

    void deallocate() {
        if (data) {
            std::free(data);
            data = nullptr;
            size = 0;
        }
    }

    bool zero() {
        if (!data) return false;
        return simd_zero_memory(data, size) == static_cast<int64_t>(size);
    }
};

static EnclaveBuffer g_enclave;

// Initialize secure enclave
int init() {
    if (!g_enclave.allocate(config::ENCLAVE_SIZE)) {
        return -1;
    }

    // Zero the enclave memory
    if (!g_enclave.zero()) {
        g_enclave.deallocate();
        return -2;
    }

    g_hv_state.enclave_base = reinterpret_cast<uint64_t>(g_enclave.data);
    g_hv_state.enclave_active = true;

    return 0;
}

// Cleanup enclave
void cleanup() {
    g_enclave.zero();  // Securely wipe before freeing
    g_enclave.deallocate();
    g_hv_state.enclave_active = false;
}

// Send data to enclave for processing
int send_data(const uint8_t* src, uint64_t size) {
    if (!g_hv_state.enclave_active || !src || size == 0) {
        return -1;
    }

    if (size > config::DATA_BUFFER_SIZE) {
        return -2;
    }

    // Use SIMD to pack data into enclave
    int64_t result = simd_pack_data(
        g_enclave.data,
        src,
        size,
        1  // Use AVX-512 if available
    );

    if (result < 0) {
        return -3;
    }

    g_hv_state.data_transferred_bytes += size;

    // Here you would process the data (encrypt, decrypt, etc.)
    // For PoC, just reverse the data
    for (uint64_t i = 0; i < size / 2; ++i) {
        std::swap(g_enclave.data[i], g_enclave.data[size - 1 - i]);
    }

    return 0;
}

// Receive processed data from enclave
int receive_data(uint8_t* dest, uint64_t size) {
    if (!g_hv_state.enclave_active || !dest || size == 0) {
        return -1;
    }

    if (size > config::DATA_BUFFER_SIZE) {
        return -2;
    }

    // Use SIMD to unpack data from enclave
    int64_t result = simd_unpack_data(
        dest,
        g_enclave.data,
        size,
        1  // Use AVX-512 if available
    );

    if (result < 0) {
        return -3;
    }

    return 0;
}

} // namespace enclave

// =============================================================================
// Monitoring and Security
// =============================================================================
namespace monitor {

int init() {
    printf("[*] Initializing trace engine...\n");

    int64_t result = trace_init();
    if (result != 0) {
        printf("[-] Trace engine initialization failed: %ld\n", result);
        return -1;
    }

    printf("[+] Trace engine initialized\n");

    // Set watchpoints on critical regions
    // In production, would monitor IDT, GDT, syscall tables, etc.

    g_hv_state.trace_enabled = true;
    return 0;
}

int check_integrity() {
    if (!g_hv_state.trace_enabled) {
        return -1;
    }

    int64_t result = trace_check_integrity();
    if (result != 0) {
        printf("[!] Integrity check FAILED - possible rootkit detected!\n");
        g_hv_state.integrity_violations++;
        return -2;
    }

    return 0;
}

void log_event(uint64_t event_type, uint64_t data) {
    if (g_hv_state.trace_enabled) {
        trace_log_event(event_type, data);
    }
}

} // namespace monitor

// =============================================================================
// Test Functions
// =============================================================================
namespace testing {

void test_simd_operations() {
    printf("\n[*] Testing SIMD operations...\n");

    constexpr uint64_t TEST_SIZE = 256;
    uint8_t src[TEST_SIZE];
    uint8_t dest[TEST_SIZE];

    // Initialize test data
    for (uint64_t i = 0; i < TEST_SIZE; ++i) {
        src[i] = static_cast<uint8_t>(i & 0xFF);
    }

    // Test SIMD memcpy
    int64_t result = simd_secure_memcpy(dest, src, TEST_SIZE);
    if (result == TEST_SIZE) {
        printf("[+] SIMD memcpy successful\n");
    } else {
        printf("[-] SIMD memcpy failed\n");
    }

    // Test hash
    uint8_t hash[32];
    result = simd_hash_blocks(src, TEST_SIZE, hash);
    if (result == 0) {
        printf("[+] SIMD hash successful\n");
    } else {
        printf("[-] SIMD hash failed\n");
    }

    // Test zero memory
    result = simd_zero_memory(dest, TEST_SIZE);
    if (result == TEST_SIZE) {
        printf("[+] SIMD zero memory successful\n");
    } else {
        printf("[-] SIMD zero memory failed\n");
    }

    // Verify zero
    bool all_zero = true;
    for (uint64_t i = 0; i < TEST_SIZE; ++i) {
        if (dest[i] != 0) {
            all_zero = false;
            break;
        }
    }
    if (all_zero) {
        printf("[+] Memory zero verified\n");
    }
}

void test_enclave_operations() {
    printf("\n[*] Testing enclave operations...\n");

    const char* test_message = "Hello, Secure Enclave!";
    constexpr uint64_t msg_len = sizeof("Hello, Secure Enclave!") - 1;

    uint8_t recv_buffer[msg_len];

    // Send data to enclave
    int result = enclave::send_data(
        reinterpret_cast<const uint8_t*>(test_message),
        msg_len
    );

    if (result == 0) {
        printf("[+] Data sent to enclave\n");
    } else {
        printf("[-] Failed to send data to enclave: %d\n", result);
        return;
    }

    // Receive processed data
    result = enclave::receive_data(recv_buffer, msg_len);

    if (result == 0) {
        printf("[+] Data received from enclave\n");
        printf("    Original: %s\n", test_message);
        printf("    Processed: ");
        for (uint64_t i = 0; i < msg_len; ++i) {
            printf("%c", recv_buffer[i]);
        }
        printf("\n");
    } else {
        printf("[-] Failed to receive data from enclave: %d\n", result);
    }
}

} // namespace testing

// =============================================================================
// Public API
// =============================================================================

extern "C" {

// Main hypervisor initialization
int hypervisor_init() {
    printf("\n");
    printf("========================================\n");
    printf("  Kernova-TEE v2.0.0\n");
    printf("  Trusted Execution Environment\n");
    printf("========================================\n\n");

    // Initialize monitoring first
    if (monitor::init() != 0) {
        printf("[-] Failed to initialize monitoring\n");
        return -1;
    }

    // Initialize enclave
    if (enclave::init() != 0) {
        printf("[-] Failed to initialize enclave\n");
        return -2;
    }

    g_backend = &kernova::virtualization::select_backend();
    auto backend_status = g_backend->status();

    printf("[*] CPU Vendor: %s (%s)\n",
           kernova::virtualization::cpu_vendor_name(backend_status.cpu.vendor),
           backend_status.cpu.vendor_id);
    printf("[*] Selected virtualization backend: %s\n", g_backend->name());

    // Initialize hardware virtualization when the host/runtime allows it.
    // Docker and normal userspace runs fall back to PoC mode so SIMD, enclave
    // and monitor flows can be validated without privileged instructions.
    if (g_backend->init() != 0) {
        if (config::strict_vmx_required()) {
            printf("[-] Failed to initialize hardware virtualization backend\n");
            return -3;
        }
        printf("[!] %s; continuing in userspace PoC mode\n", backend_status.reason);
    } else if (g_backend->setup_guest() != 0) {
        if (config::strict_vmx_required()) {
            printf("[-] Failed to setup hardware virtualization guest state\n");
            return -4;
        }
        printf("[!] Guest setup unavailable; continuing in userspace PoC mode\n");
    } else {
        if (config::hardware_backend_enabled() && backend_status.kernel_driver_available) {
            printf("[*] Attempting privileged kernel driver launch...\n");
            if (g_backend->launch() != 0) {
                if (config::strict_vmx_required()) {
                    printf("[-] Kernel driver launch failed\n");
                    return -5;
                }
                printf("[!] Kernel driver launch did not complete; continuing in PoC mode\n");
            }
        }
        g_hv_state.hardware_virtualization_validated = g_backend->status().hardware_validated;
    }

    printf("\n[+] Hypervisor initialization complete\n");
    return 0;
}

// Main hypervisor loop
void hypervisor_main() {
    printf("\n[*] Entering hypervisor main loop...\n");

    // Run tests
    testing::test_simd_operations();
    testing::test_enclave_operations();

    // Periodic integrity check
    printf("\n[*] Running integrity check...\n");
    if (monitor::check_integrity() == 0) {
        printf("[+] System integrity verified\n");
    }

    // In production, would launch the guest here
    // For PoC, we'll just halt
    printf("\n[*] Hypervisor ready - halting (PoC mode)\n");
    printf("    In production, guest OS would resume here\n\n");

    // Statistics
    printf("========================================\n");
    printf("  Statistics:\n");
    printf("  - Backend: %s\n", g_backend ? g_backend->name() : "Unknown");
    printf("  - Hardware Virtualization: %s\n",
           g_hv_state.hardware_virtualization_validated ? "Validated" : "PoC/Fallback");
    printf("  - Enclave Active: %s\n",
           g_hv_state.enclave_active ? "Yes" : "No");
    printf("  - Data Transferred: %lu bytes\n",
           g_hv_state.data_transferred_bytes);
    printf("========================================\n\n");

    if (!g_hv_state.hardware_virtualization_validated) {
        printf("[*] Userspace PoC run complete\n");
        return;
    }

    // Halt - in production would be in VM loop
    while (true) {
        asm volatile("hlt");
    }
}

// Cleanup and shutdown
void hypervisor_shutdown() {
    printf("\n[*] Shutting down hypervisor...\n");

    if (g_backend) {
        g_backend->shutdown();
    }
    enclave::cleanup();

    printf("[+] Hypervisor shutdown complete\n");
}

// VMCALL handler - guest requests to hypervisor
uint64_t vmcall_handler(uint64_t function, uint64_t param1, uint64_t param2) {
    switch (function) {
        case 0:  // Echo test
            printf("[VMCALL] Echo: %lu, %lu\n", param1, param2);
            return param1 + param2;

        case 1:  // Enclave operation
            return enclave::send_data(
                reinterpret_cast<const uint8_t*>(param1),
                param2
            );

        case 2:  // Integrity check
            return monitor::check_integrity();

        default:
            printf("[VMCALL] Unknown function: %lu\n", function);
            return 0xFFFFFFFFFFFFFFFFULL;
    }
}

} // extern "C"

// =============================================================================
// Entry Point (for standalone testing)
// =============================================================================
int main() {
    int result = hypervisor_init();
    if (result != 0) {
        printf("Fatal: Hypervisor initialization failed with code %d\n", result);
        return result;
    }

    hypervisor_main();

    // Should not reach here in production
    hypervisor_shutdown();

    return 0;
}
