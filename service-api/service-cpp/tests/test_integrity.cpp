// =============================================================================
// Integrity Checking Test
// =============================================================================

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

// External trace/integrity functions
extern "C" {
    int64_t trace_init();
    int64_t trace_log_event(uint64_t event_type, uint64_t data);
    int64_t trace_check_integrity();
    int64_t trace_set_watchpoint(uint64_t address, uint64_t size, uint64_t type);
    int64_t simd_hash_blocks(const uint8_t* data, uint64_t size, uint8_t* hash_out);
}

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

// Test 1: Hash Determinism
void test_hash_determinism() {
    printf("[*] Testing hash determinism...\n");

    constexpr size_t DATA_SIZE = 256;
    uint8_t data[DATA_SIZE];
    uint8_t hash1[32];
    uint8_t hash2[32];

    // Initialize data
    for (size_t i = 0; i < DATA_SIZE; ++i) {
        data[i] = static_cast<uint8_t>(i);
    }

    // Compute hash twice
    int64_t result1 = simd_hash_blocks(data, DATA_SIZE, hash1);
    int64_t result2 = simd_hash_blocks(data, DATA_SIZE, hash2);

    if (result1 != 0 || result2 != 0) {
        test_failed("Hash determinism", "Hash function failed");
        return;
    }

    // Hashes must be identical
    bool match = (std::memcmp(hash1, hash2, 32) == 0);

    if (match) {
        test_passed("Hash determinism");
    } else {
        test_failed("Hash determinism", "Hashes differ for same input");
    }
}

// Test 2: Hash Avalanche Effect
void test_hash_avalanche() {
    printf("[*] Testing hash avalanche effect...\n");

    constexpr size_t DATA_SIZE = 256;
    uint8_t data1[DATA_SIZE];
    uint8_t data2[DATA_SIZE];
    uint8_t hash1[32];
    uint8_t hash2[32];

    // Initialize data with minimal difference
    for (size_t i = 0; i < DATA_SIZE; ++i) {
        data1[i] = static_cast<uint8_t>(i);
        data2[i] = static_cast<uint8_t>(i);
    }

    // Flip one bit
    data2[0] ^= 0x01;

    // Compute hashes
    simd_hash_blocks(data1, DATA_SIZE, hash1);
    simd_hash_blocks(data2, DATA_SIZE, hash2);

    // Count differing bits
    int diff_bits = 0;
    for (size_t i = 0; i < 32; ++i) {
        uint8_t diff = hash1[i] ^ hash2[i];
        while (diff) {
            diff_bits += diff & 1;
            diff >>= 1;
        }
    }

    // Good hash should have ~50% of bits different (128 bits expected)
    // We'll be lenient and accept at least 64 bits different
    if (diff_bits >= 64) {
        test_passed("Hash avalanche effect");
    } else {
        char reason[128];
        snprintf(reason, sizeof(reason), "Insufficient avalanche: %d bits different", diff_bits);
        test_failed("Hash avalanche effect", reason);
    }
}

// Test 3: Hash Collision Resistance (Basic)
void test_hash_collision() {
    printf("[*] Testing hash collision resistance...\n");

    constexpr size_t DATA_SIZE = 64;
    constexpr size_t NUM_SAMPLES = 1000;

    std::vector<std::vector<uint8_t>> hashes;

    for (size_t i = 0; i < NUM_SAMPLES; ++i) {
        uint8_t data[DATA_SIZE];
        uint8_t hash[32];

        // Initialize data with pattern
        for (size_t j = 0; j < DATA_SIZE; ++j) {
            data[j] = static_cast<uint8_t>((i + j) & 0xFF);
        }

        simd_hash_blocks(data, DATA_SIZE, hash);

        // Store hash
        std::vector<uint8_t> hash_vec(hash, hash + 32);
        hashes.push_back(hash_vec);
    }

    // Check for collisions
    bool collision = false;
    for (size_t i = 0; i < hashes.size() && !collision; ++i) {
        for (size_t j = i + 1; j < hashes.size(); ++j) {
            if (hashes[i] == hashes[j]) {
                collision = true;
                break;
            }
        }
    }

    if (!collision) {
        test_passed("Hash collision resistance");
    } else {
        test_failed("Hash collision resistance", "Collision detected");
    }
}

// Test 4: Event Logging
void test_event_logging() {
    printf("[*] Testing event logging...\n");

    // Initialize trace engine
    int64_t result = trace_init();
    if (result != 0) {
        test_failed("Event logging", "Trace init failed");
        return;
    }

    // Log some events
    for (int i = 0; i < 10; ++i) {
        trace_log_event(i, i * 100);
    }

    // If we got here without crashing, logging works
    test_passed("Event logging");
}

// Test 5: Watchpoint Setting
void test_watchpoint() {
    printf("[*] Testing watchpoint configuration...\n");

    // Set a watchpoint
    constexpr uint64_t TEST_ADDR = 0x1000;
    constexpr uint64_t TEST_SIZE = 8;
    constexpr uint64_t TEST_TYPE = 0;  // Write watchpoint

    int64_t result = trace_set_watchpoint(TEST_ADDR, TEST_SIZE, TEST_TYPE);

    if (result == 0) {
        test_passed("Watchpoint configuration");
    } else {
        test_failed("Watchpoint configuration", "Failed to set watchpoint");
    }
}

// Test 6: Memory Integrity Check
void test_memory_integrity() {
    printf("[*] Testing memory integrity check...\n");

    constexpr size_t BUFFER_SIZE = 1024;
    uint8_t buffer1[BUFFER_SIZE];
    uint8_t buffer2[BUFFER_SIZE];
    uint8_t hash1[32];
    uint8_t hash2[32];

    // Initialize both buffers identically
    for (size_t i = 0; i < BUFFER_SIZE; ++i) {
        buffer1[i] = static_cast<uint8_t>(i & 0xFF);
        buffer2[i] = static_cast<uint8_t>(i & 0xFF);
    }

    // Compute initial hash
    simd_hash_blocks(buffer1, BUFFER_SIZE, hash1);

    // Verify integrity
    simd_hash_blocks(buffer2, BUFFER_SIZE, hash2);

    if (std::memcmp(hash1, hash2, 32) == 0) {
        test_passed("Memory integrity check (intact)");
    } else {
        test_failed("Memory integrity check (intact)", "Hashes differ for identical data");
    }

    // Now modify buffer2
    buffer2[100] ^= 0xFF;

    // Recompute hash
    simd_hash_blocks(buffer2, BUFFER_SIZE, hash2);

    // Integrity should fail
    if (std::memcmp(hash1, hash2, 32) != 0) {
        test_passed("Memory integrity check (modified)");
    } else {
        test_failed("Memory integrity check (modified)", "Failed to detect modification");
    }
}

// Test 7: Regional Integrity
void test_regional_integrity() {
    printf("[*] Testing regional integrity monitoring...\n");

    // Simulate monitoring different memory regions
    constexpr size_t NUM_REGIONS = 5;
    constexpr size_t REGION_SIZE = 256;

    struct Region {
        uint64_t tag;
        uint8_t data[REGION_SIZE];
        uint8_t hash[32];
    } regions[NUM_REGIONS];

    // Initialize regions
    for (size_t r = 0; r < NUM_REGIONS; ++r) {
        for (size_t i = 0; i < REGION_SIZE; ++i) {
            regions[r].data[i] = static_cast<uint8_t>((r * REGION_SIZE + i) & 0xFF);
        }
        regions[r].tag = r;

        // Compute baseline hash
        simd_hash_blocks(regions[r].data, REGION_SIZE, regions[r].hash);
    }

    // Verify all regions
    bool all_valid = true;
    for (size_t r = 0; r < NUM_REGIONS; ++r) {
        uint8_t current_hash[32];
        simd_hash_blocks(regions[r].data, REGION_SIZE, current_hash);

        if (std::memcmp(current_hash, regions[r].hash, 32) != 0) {
            all_valid = false;
            break;
        }
    }

    if (all_valid) {
        test_passed("Regional integrity (baseline)");
    } else {
        test_failed("Regional integrity (baseline)", "Baseline mismatch");
    }

    // Modify one region
    regions[2].data[50] ^= 0xAA;

    // Recompute hash for modified region
    uint8_t new_hash[32];
    simd_hash_blocks(regions[2].data, REGION_SIZE, new_hash);

    // Should detect modification
    if (std::memcmp(new_hash, regions[2].hash, 32) != 0) {
        test_passed("Regional integrity (detection)");
    } else {
        test_failed("Regional integrity (detection)", "Failed to detect region modification");
    }
}

int main() {
    printf("\n");
    printf("========================================\n");
    printf("  Integrity Checking Test Suite\n");
    printf("========================================\n\n");

    // Run tests
    test_hash_determinism();
    test_hash_avalanche();
    test_hash_collision();
    test_event_logging();
    test_watchpoint();
    test_memory_integrity();
    test_regional_integrity();

    // Summary
    printf("\n");
    printf("========================================\n");
    printf("  Test Results:\n");
    printf("  Passed: %d\n", stats.passed);
    printf("  Failed: %d\n", stats.failed);
    printf("========================================\n\n");

    return (stats.failed == 0) ? 0 : 1;
}
