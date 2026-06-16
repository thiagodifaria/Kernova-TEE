# =============================================================================
# Kernova-TEE Tests
# =============================================================================
# Tests link against kernova_asm_testable (SIMD + trace engine ASM).
# These ASM modules contain pure computation functions safe for userspace.
#
# Tests do NOT link kernova_asm_boot (entry.asm, vmx_init, vmcs_config,
# intercept) because those contain _start, VMX instructions, and
# privileged operations that only work in Ring -1.
# =============================================================================

# Include directories for all tests
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../../shared)

# =============================================================================
# Test: SIMD Operations (links testable ASM)
# =============================================================================
add_executable(test_simd
    test_simd.cpp
    $<TARGET_OBJECTS:kernova_asm_testable>
)
target_compile_options(test_simd PRIVATE -Wall -Wextra -mavx2 -fno-pie)
target_link_options(test_simd PRIVATE -no-pie)
add_test(NAME test_simd COMMAND test_simd)

# =============================================================================
# Test: VMCS Configuration (standalone — no ASM linkage)
# =============================================================================
add_executable(test_vmcs
    test_vmcs.cpp
)
target_compile_options(test_vmcs PRIVATE -Wall -Wextra)
add_test(NAME test_vmcs COMMAND test_vmcs)

# =============================================================================
# Test: Marshalling Performance (links testable ASM)
# =============================================================================
add_executable(test_performance
    test_performance.cpp
    $<TARGET_OBJECTS:kernova_asm_testable>
)
target_compile_options(test_performance PRIVATE -O3 -march=native -mavx2 -fno-pie)
target_link_options(test_performance PRIVATE -no-pie)
add_test(NAME test_performance COMMAND test_performance)

# =============================================================================
# Test: Integrity Checking (links testable ASM)
# =============================================================================
add_executable(test_integrity
    test_integrity.cpp
    $<TARGET_OBJECTS:kernova_asm_testable>
)
target_compile_options(test_integrity PRIVATE -Wall -Wextra -mavx2 -fno-pie)
target_link_options(test_integrity PRIVATE -no-pie)
add_test(NAME test_integrity COMMAND test_integrity)

# =============================================================================
# Test: Virtualization Backend Selection (userspace-safe C++)
# =============================================================================
add_executable(test_backend
    test_backend.cpp
    ${CMAKE_SOURCE_DIR}/src/virtualization/cpu_features.cpp
    ${CMAKE_SOURCE_DIR}/src/virtualization/hypervisor_backend.cpp
    ${CMAKE_SOURCE_DIR}/src/virtualization/amd_svm/amd_svm.cpp
    ${CMAKE_SOURCE_DIR}/src/vmx_stubs.cpp
)
target_compile_options(test_backend PRIVATE -Wall -Wextra)
add_test(NAME test_backend COMMAND test_backend)

# =============================================================================
# Test: AMD SVM Capability Model (userspace-safe C++)
# =============================================================================
add_executable(test_amd_svm
    test_amd_svm.cpp
    ${CMAKE_SOURCE_DIR}/src/virtualization/cpu_features.cpp
    ${CMAKE_SOURCE_DIR}/src/virtualization/amd_svm/amd_svm.cpp
)
target_compile_options(test_amd_svm PRIVATE -Wall -Wextra)
add_test(NAME test_amd_svm COMMAND test_amd_svm)
