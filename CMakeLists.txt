# =============================================================================
# Micro-Hypervisor Build Configuration
# =============================================================================
# CMake build system for the x86-64 Micro-Hypervisor project
# =============================================================================

cmake_minimum_required(VERSION 3.20)
project(MicroHypervisor
    VERSION 1.0.0
    DESCRIPTION "Trusted Execution Environment via Micro-Hypervisor"
    LANGUAGES ASM CXX
)

# =============================================================================
# Project Options
# =============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)  # For Linux/ELF
# For Windows, use: set(CMAKE_ASM_NASM_OBJECT_FORMAT win64)

# =============================================================================
# Build Type
# =============================================================================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# =============================================================================
# Compiler Flags
# =============================================================================
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")

set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -f elf64")

# =============================================================================
# Source Files
# =============================================================================
set(ASM_SOURCES
    src/boot/entry.asm
    src/core/vmx_init.asm
    src/core/vmcs_config.asm
    src/marshall/simd_packer.asm
    src/monitor/intercept.asm
    src/monitor/trace_engine.asm
)

set(CPP_SOURCES
    src/main.cpp
)

set(HEADERS
    include/vmx_defs.inc
    include/avx_macros.inc
    include/registers.hpp
)

# =============================================================================
# Executable Target
# =============================================================================
add_executable(${PROJECT_NAME} ${ASM_SOURCES} ${CPP_SOURCES} ${HEADERS})

# =============================================================================
# Include Directories
# =============================================================================
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# =============================================================================
# Linker Options
# =============================================================================
target_link_options(${PROJECT_NAME} PRIVATE
    -nostdlib
    -static
    -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
)

# =============================================================================
# Link Libraries
# =============================================================================
# For a bare-metal hypervisor, we typically don't link standard libraries
# If you need libgcc for intrinsics:
# target_link_libraries(${PROJECT_NAME} PRIVATE gcc)

# =============================================================================
# Installation
# =============================================================================
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(FILES ${HEADERS}
    DESTINATION include/${PROJECT_NAME}
)

# =============================================================================
# Testing
# =============================================================================
enable_testing()

add_subdirectory(tests)

# =============================================================================
# Documentation
# =============================================================================
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()

# =============================================================================
# Print Configuration Summary
# =============================================================================
message(STATUS "")
message(STATUS "==================== Configuration Summary ====================")
message(STATUS "Project:         ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Build type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler:    ${CMAKE_CXX_COMPILER}")
message(STATUS "ASM compiler:    ${CMAKE_ASM_NASM_COMPILER}")
message(STATUS "Install prefix:  ${CMAKE_INSTALL_PREFIX}")
message(STATUS "==============================================================")
message(STATUS "")

# =============================================================================
# Custom Targets
# =============================================================================
# Run the hypervisor in QEMU
add_custom_target(run-qemu
    COMMAND qemu-system-x86_64
        -enable-kvm
        -m 2G
        -nographic
        -kernel ${PROJECT_NAME}
        -serial mon:stdio
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running hypervisor in QEMU with KVM acceleration"
    VERBATIM
)

# Run with GDB debugging
add_custom_target(debug-qemu
    COMMAND qemu-system-x86_64
        -enable-kvm
        -m 2G
        -nographic
        -kernel ${PROJECT_NAME}
        -serial mon:stdio
        -s -S
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running hypervisor in QEMU with GDB server (port 1234)"
    VERBATIM
)

# Generate ISO image for booting
add_custom_target(iso
    COMMAND mkdir -p iso/boot/grub
    COMMAND cp ${PROJECT_NAME} iso/boot/${PROJECT_NAME}.bin
    COMMAND echo > iso/boot/grub/grub.cfg
    COMMAND echo 'menuentry "Micro-Hypervisor" {' >> iso/boot/grub/grub.cfg
    COMMAND echo '    multiboot2 /boot/${PROJECT_NAME}.bin' >> iso/boot/grub/grub.cfg
    COMMAND echo '    boot' >> iso/boot/grub/grub.cfg
    COMMAND echo '}' >> iso/boot/grub/grub.cfg
    COMMAND grub-mkrescue -o ${PROJECT_NAME}.iso iso
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Creating bootable ISO image"
    VERBATIM
)

# =============================================================================
# Sanitizers (for debug builds)
# =============================================================================
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Note: Address sanitizer may not work well with bare-metal/VMX code
    # Use with caution
    # target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
    # target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
endif()

# =============================================================================
# Warnings as Errors (optional)
# =============================================================================
option(WERROR "Treat warnings as errors" OFF)
if(WERROR)
    target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
endif()
