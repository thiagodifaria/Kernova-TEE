# =============================================================================
# Micro-Hypervisor Tests
# =============================================================================

# Test: SIMD Operations
add_executable(test_simd
    test_simd.cpp
)

target_link_libraries(test_simd PRIVATE
    # Link against main project if building as library
    # For now, standalone
)

add_test(NAME test_simd COMMAND test_simd)

# Test: VMCS Configuration
add_executable(test_vmcs
    test_vmcs.cpp
)

add_test(NAME test_vmcs COMMAND test_vmcs)

# Test: Marshalling Performance
add_executable(test_performance
    test_performance.cpp
)

target_compile_options(test_performance PRIVATE
    -O3
    -march=native
)

add_test(NAME test_performance COMMAND test_performance)

# Test: Integrity Checking
add_executable(test_integrity
    test_integrity.cpp
)

add_test(NAME test_integrity COMMAND test_integrity)
