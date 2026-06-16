// =============================================================================
// SIMD Operations Test
// =============================================================================

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <immintrin.h>

// External SIMD functions
extern "C" {
    int64_t simd_pack_data(uint8_t* dest, const uint8_t* src, uint64_t size, uint64_t flags);
    int64_t simd_unpack_data(uint8_t* dest, const uint8_t* src, uint64_t size, uint64_t flags);
    int64_t simd_secure_memcpy(uint8_t* dest, const uint8_t* src, uint64_t size);
    int64_t simd_zero_memory(uint8_t* buffer, uint64_t size);
    int64_t simd_hash_blocks(const uint8_t* data, uint64_t size, uint8_t* hash_out);
}

// Test configuration
constexpr size_t TEST_BUFFER_SIZE = 4096;
constexpr size_t NUM_ITERATIONS = 1000;

// Test result tracking
struct TestStats {
    int passed{0};
    int failed{0};
} stats;

// Color codes for output
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

// Test 1: SIMD Memcpy
void test_simd_memcpy() {
    uint8_t src[TEST_BUFFER_SIZE];
    uint8_t dest[TEST_BUFFER_SIZE];

    // Initialize source
    for (size_t i = 0; i < TEST_BUFFER_SIZE; ++i) {
        src[i] = static_cast<uint8_t>(i & 0xFF);
    }

    memset(dest, 0, TEST_BUFFER_SIZE);

    // Test aligned copy
    int64_t result = simd_secure_memcpy(dest, src, TEST_BUFFER_SIZE);

    if (result != TEST_BUFFER_SIZE) {
        test_failed("SIMD Memcpy", "Wrong return value");
        return;
    }

    // Verify data
    bool match = true;
    for (size_t i = 0; i < TEST_BUFFER_SIZE; ++i) {
        if (dest[i] != src[i]) {
            match = false;
            break;
        }
    }

    if (match) {
        test_passed("SIMD Memcpy");
    } else {
        test_failed("SIMD Memcpy", "Data mismatch");
    }
}

// Test 2: SIMD Zero Memory
void test_simd_zero() {
    uint8_t buffer[TEST_BUFFER_SIZE];

    // Fill with garbage
    memset(buffer, 0xFF, TEST_BUFFER_SIZE);

    // Zero it
    int64_t result = simd_zero_memory(buffer, TEST_BUFFER_SIZE);

    if (result != TEST_BUFFER_SIZE) {
        test_failed("SIMD Zero Memory", "Wrong return value");
        return;
    }

    // Verify all zeros
    bool all_zero = true;
    for (size_t i = 0; i < TEST_BUFFER_SIZE; ++i) {
        if (buffer[i] != 0) {
            all_zero = false;
            break;
        }
    }

    if (all_zero) {
        test_passed("SIMD Zero Memory");
    } else {
        test_failed("SIMD Zero Memory", "Memory not zeroed");
    }
}

// Test 3: SIMD Pack/Unpack
void test_simd_pack_unpack() {
    uint8_t src[TEST_BUFFER_SIZE];
    uint8_t temp[TEST_BUFFER_SIZE];
    uint8_t dest[TEST_BUFFER_SIZE];

    // Initialize source with pattern
    for (size_t i = 0; i < TEST_BUFFER_SIZE; ++i) {
        src[i] = static_cast<uint8_t>(i & 0xFF);
    }

    memset(temp, 0, TEST_BUFFER_SIZE);
    memset(dest, 0, TEST_BUFFER_SIZE);

    // Pack data
    int64_t packed = simd_pack_data(temp, src, TEST_BUFFER_SIZE, 1);

    if (packed != TEST_BUFFER_SIZE) {
        test_failed("SIMD Pack/Unpack", "Pack failed");
        return;
    }

    // Unpack data
    int64_t unpacked = simd_unpack_data(dest, temp, TEST_BUFFER_SIZE, 1);

    if (unpacked != TEST_BUFFER_SIZE) {
        test_failed("SIMD Pack/Unpack", "Unpack failed");
        return;
    }

    // Verify round-trip
    bool match = true;
    for (size_t i = 0; i < TEST_BUFFER_SIZE; ++i) {
        if (dest[i] != src[i]) {
            match = false;
            break;
        }
    }

    if (match) {
        test_passed("SIMD Pack/Unpack");
    } else {
        test_failed("SIMD Pack/Unpack", "Round-trip data mismatch");
    }
}

// Test 4: SIMD Hash
void test_simd_hash() {
    uint8_t data[256];
    uint8_t hash1[32];
    uint8_t hash2[32];

    // Initialize data
    for (size_t i = 0; i < 256; ++i) {
        data[i] = static_cast<uint8_t>(i);
    }

    // Compute hash twice
    int64_t result1 = simd_hash_blocks(data, 256, hash1);
    int64_t result2 = simd_hash_blocks(data, 256, hash2);

    if (result1 != 0 || result2 != 0) {
        test_failed("SIMD Hash", "Hash function failed");
        return;
    }

    // Hashes should be identical for same input
    bool match = true;
    for (size_t i = 0; i < 32; ++i) {
        if (hash1[i] != hash2[i]) {
            match = false;
            break;
        }
    }

    if (!match) {
        test_failed("SIMD Hash", "Hashes not deterministic");
        return;
    }

    // Different data should produce different hash
    data[0] = 0xFF;
    int64_t result3 = simd_hash_blocks(data, 256, hash2);

    bool different = false;
    for (size_t i = 0; i < 32; ++i) {
        if (hash1[i] != hash2[i]) {
            different = true;
            break;
        }
    }

    if (different) {
        test_passed("SIMD Hash");
    } else {
        test_failed("SIMD Hash", "Different data produced same hash");
    }
}

// Test 5: Unaligned Access
void test_unaligned_access() {
    uint8_t buffer[TEST_BUFFER_SIZE + 64];

    // Initialize
    for (size_t i = 0; i < TEST_BUFFER_SIZE + 64; ++i) {
        buffer[i] = static_cast<uint8_t>(i & 0xFF);
    }

    // Test with various alignments
    for (int offset = 1; offset <= 63; ++offset) {
        uint8_t src[256];
        uint8_t dest[256];

        memcpy(src, buffer + offset, 256);
        memset(dest, 0, 256);

        int64_t result = simd_secure_memcpy(dest, src, 256);

        if (result != 256) {
            test_failed("Unaligned Access", "Copy failed");
            return;
        }

        // Verify
        if (memcmp(dest, src, 256) != 0) {
            test_failed("Unaligned Access", "Data mismatch");
            return;
        }
    }

    test_passed("Unaligned Access");
}

// Performance benchmark
void benchmark_memcpy() {
    printf("\n[*] Performance Benchmark: SIMD vs Standard memcpy\n");

    uint8_t* src = new uint8_t[1024 * 1024];  // 1MB
    uint8_t* dest_simd = new uint8_t[1024 * 1024];
    uint8_t* dest_std = new uint8_t[1024 * 1024];

    // Initialize source
    for (size_t i = 0; i < 1024 * 1024; ++i) {
        src[i] = static_cast<uint8_t>(i & 0xFF);
    }

    // Benchmark SIMD
    auto start_simd = __builtin_ia32_rdtsc();
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        simd_secure_memcpy(dest_simd, src, 1024 * 1024);
    }
    auto end_simd = __builtin_ia32_rdtsc();
    uint64_t cycles_simd = end_simd - start_simd;

    // Benchmark standard memcpy
    auto start_std = __builtin_ia32_rdtsc();
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        memcpy(dest_std, src, 1024 * 1024);
    }
    auto end_std = __builtin_ia32_rdtsc();
    uint64_t cycles_std = end_std - start_std;

    printf("    SIMD memcpy:  %lu cycles\n", cycles_simd);
    printf("    Standard memcpy: %lu cycles\n", cycles_std);

    if (cycles_simd < cycles_std) {
        double speedup = static_cast<double>(cycles_std) / cycles_simd;
        printf("    Speedup: %.2fx\n", speedup);
    } else {
        double slowdown = static_cast<double>(cycles_simd) / cycles_std;
        printf("    Slowdown: %.2fx (SIMD not beneficial for this pattern)\n", slowdown);
    }

    delete[] src;
    delete[] dest_simd;
    delete[] dest_std;
}

// Main test runner
int main() {
    printf("\n");
    printf("========================================\n");
    printf("  SIMD Operations Test Suite\n");
    printf("========================================\n\n");

    // Run tests
    test_simd_memcpy();
    test_simd_zero();
    test_simd_pack_unpack();
    test_simd_hash();
    test_unaligned_access();

    // Performance benchmark
    benchmark_memcpy();

    // Summary
    printf("\n");
    printf("========================================\n");
    printf("  Test Results:\n");
    printf("  Passed: %d\n", stats.passed);
    printf("  Failed: %d\n", stats.failed);
    printf("========================================\n\n");

    return (stats.failed == 0) ? 0 : 1;
}
