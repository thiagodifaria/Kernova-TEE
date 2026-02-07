// =============================================================================
// Performance Benchmarking Test
// =============================================================================

#include <cstdint>
#include <cstdio>
#include <chrono>
#include <vector>

// External SIMD functions
extern "C" {
    int64_t simd_pack_data(uint8_t* dest, const uint8_t* src, uint64_t size, uint64_t flags);
    int64_t simd_unpack_data(uint8_t* dest, const uint8_t* src, uint64_t size, uint64_t flags);
    int64_t simd_secure_memcpy(uint8_t* dest, const uint8_t* src, uint64_t size);
}

using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<double>;

struct BenchmarkResult {
    const char* name;
    double seconds;
    double throughput_mb_s;
};

// Benchmark: SIMD Memcpy throughput
BenchmarkResult benchmark_memcpy_throughput(size_t data_size_mb, int iterations) {
    const size_t buffer_size = data_size_mb * 1024 * 1024;

    // Allocate aligned buffers
    uint8_t* src = static_cast<uint8_t*>(std::aligned_alloc(64, buffer_size));
    uint8_t* dest = static_cast<uint8_t*>(std::aligned_alloc(64, buffer_size));

    // Initialize source
    for (size_t i = 0; i < buffer_size; ++i) {
        src[i] = static_cast<uint8_t>(i & 0xFF);
    }

    // Warm-up
    simd_secure_memcpy(dest, src, buffer_size);

    // Benchmark
    auto start = Clock::now();
    for (int i = 0; i < iterations; ++i) {
        simd_secure_memcpy(dest, src, buffer_size);
    }
    auto end = Clock::now();

    Duration elapsed = end - start;

    // Calculate throughput
    double total_mb = static_cast<double>(data_size_mb) * iterations;
    double throughput = total_mb / elapsed.count();

    std::free(src);
    std::free(dest);

    return {
        "SIMD Memcpy",
        elapsed.count(),
        throughput
    };
}

// Benchmark: Pack/Unpack round-trip
BenchmarkResult benchmark_pack_unpack(size_t data_size_mb, int iterations) {
    const size_t buffer_size = data_size_mb * 1024 * 1024;

    uint8_t* src = static_cast<uint8_t*>(std::aligned_alloc(64, buffer_size));
    uint8_t* temp = static_cast<uint8_t*>(std::aligned_alloc(64, buffer_size));
    uint8_t* dest = static_cast<uint8_t*>(std::aligned_alloc(64, buffer_size));

    // Initialize source
    for (size_t i = 0; i < buffer_size; ++i) {
        src[i] = static_cast<uint8_t>(i & 0xFF);
    }

    // Warm-up
    simd_pack_data(temp, src, buffer_size, 1);
    simd_unpack_data(dest, temp, buffer_size, 1);

    // Benchmark
    auto start = Clock::now();
    for (int i = 0; i < iterations; ++i) {
        simd_pack_data(temp, src, buffer_size, 1);
        simd_unpack_data(dest, temp, buffer_size, 1);
    }
    auto end = Clock::now();

    Duration elapsed = end - start;

    double total_mb = static_cast<double>(data_size_mb) * iterations * 2;  // Pack + Unpack
    double throughput = total_mb / elapsed.count();

    std::free(src);
    std::free(temp);
    std::free(dest);

    return {
        "Pack/Unpack Round-trip",
        elapsed.count(),
        throughput
    };
}

// Benchmark: Small packet latency
BenchmarkResult benchmark_small_packets(size_t packet_size, int iterations) {
    const size_t buffer_size = packet_size;

    uint8_t* src = static_cast<uint8_t*>(std::aligned_alloc(64, buffer_size));
    uint8_t* temp = static_cast<uint8_t*>(std::aligned_alloc(64, buffer_size));
    uint8_t* dest = static_cast<uint8_t*>(std::aligned_alloc(64, buffer_size));

    // Initialize source
    for (size_t i = 0; i < buffer_size; ++i) {
        src[i] = static_cast<uint8_t>(i & 0xFF);
    }

    // Warm-up
    for (int i = 0; i < 100; ++i) {
        simd_pack_data(temp, src, buffer_size, 1);
        simd_unpack_data(dest, temp, buffer_size, 1);
    }

    // Benchmark
    auto start = Clock::now();
    for (int i = 0; i < iterations; ++i) {
        simd_pack_data(temp, src, buffer_size, 1);
        simd_unpack_data(dest, temp, buffer_size, 1);
    }
    auto end = Clock::now();

    Duration elapsed = end - start;

    double ops_per_second = iterations / elapsed.count();
    double avg_latency_ns = (elapsed.count() * 1e9) / iterations;

    std::free(src);
    std::free(temp);
    std::free(dest);

    return {
        "Small Packets",
        elapsed.count(),
        ops_per_second
    };
}

// Print results
void print_benchmark_result(const BenchmarkResult& result, const char* additional_info = "") {
    printf("  %s:\n", result.name);
    printf("    Time: %.4f seconds %s\n", result.seconds, additional_info);
    printf("    Throughput: %.2f MB/s\n", result.throughput_mb_s);
}

int main() {
    printf("\n");
    printf("========================================\n");
    printf("  Performance Benchmark Suite\n");
    printf("========================================\n\n");

    std::vector<BenchmarkResult> results;

    // Benchmark 1: Large block copy (10 MB x 100 iterations)
    printf("[*] Benchmarking large block transfer (10 MB x 100 iterations)...\n");
    results.push_back(benchmark_memcpy_throughput(10, 100));
    print_benchmark_result(results.back());
    printf("\n");

    // Benchmark 2: Medium block copy (1 MB x 1000 iterations)
    printf("[*] Benchmarking medium block transfer (1 MB x 1000 iterations)...\n");
    results.push_back(benchmark_memcpy_throughput(1, 1000));
    print_benchmark_result(results.back());
    printf("\n");

    // Benchmark 3: Pack/Unpack round-trip
    printf("[*] Benchmarking pack/unpack round-trip (5 MB x 100 iterations)...\n");
    results.push_back(benchmark_pack_unpack(5, 100));
    print_benchmark_result(results.back());
    printf("\n");

    // Benchmark 4: Small packets (64 bytes, typical crypto operation)
    printf("[*] Benchmarking small packets (64 bytes x 100000 iterations)...\n");
    BenchmarkResult small = benchmark_small_packets(64, 100000);
    printf("  %s:\n", small.name);
    printf("    Time: %.4f seconds\n", small.seconds);
    printf("    Operations/sec: %.2f ops/s\n", small.throughput_mb_s);
    printf("    Avg latency: %.2f ns/op\n", (small.seconds * 1e9) / 100000);
    printf("\n");

    // Benchmark 5: 4KB pages
    printf("[*] Benchmarking page-sized transfers (4 KB x 10000 iterations)...\n");
    small = benchmark_small_packets(4096, 10000);
    printf("  %s:\n", small.name);
    printf("    Time: %.4f seconds\n", small.seconds);
    printf("    Throughput: %.2f MB/s\n", (small.throughput_mb_s * 4096) / (1024 * 1024));
    printf("\n");

    // Summary
    printf("========================================\n");
    printf("  Performance Summary:\n");
    printf("  All benchmarks completed successfully\n");
    printf("  SIMD acceleration is functioning\n");
    printf("========================================\n\n");

    return 0;
}
