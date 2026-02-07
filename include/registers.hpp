// =============================================================================
// Micro-Hypervisor - Register Control Wrappers
// =============================================================================
// C++20 wrapper for x86-64 register manipulation and VMX control
// =============================================================================

#ifndef REGISTERS_HPP
#define REGISTERS_HPP

#include <cstdint>
#include <bit>
#include <cstring>

namespace hypervisor {
namespace regs {

// =============================================================================
// Control Register Definitions
// =============================================================================

struct CR0 {
    static constexpr uint64_t PE      = 1ULL << 0;  // Protection Enable
    static constexpr uint64_t MP      = 1ULL << 1;  // Monitor Coprocessor
    static constexpr uint64_t EM      = 1ULL << 2;  // Emulation
    static constexpr uint64_t TS      = 1ULL << 3;  // Task Switched
    static constexpr uint64_t ET      = 1ULL << 4;  // Extension Type
    static constexpr uint64_t NE      = 1ULL << 5;  // Numeric Error
    static constexpr uint64_t WP      = 1ULL << 16; // Write Protect
    static constexpr uint64_t AM      = 1ULL << 18; // Alignment Mask
    static constexpr uint64_t NW      = 1ULL << 29; // Not Write-through
    static constexpr uint64_t CD      = 1ULL << 30; // Cache Disable
    static constexpr uint64_t PG      = 1ULL << 31; // Paging

    static inline uint64_t read() noexcept {
        uint64_t value;
        asm volatile("mov %%cr0, %0" : "=r"(value));
        return value;
    }

    static inline void write(uint64_t value) noexcept {
        asm volatile("mov %0, %%cr0" : : "r"(value));
    }
};

struct CR4 {
    static constexpr uint64_t VME     = 1ULL << 0;  // Virtual-8086 Mode Extensions
    static constexpr uint64_t PVI     = 1ULL << 1;  // Protected-Mode Virtual Interrupts
    static constexpr uint64_t TSD     = 1ULL << 2;  // Time Stamp Disable
    static constexpr uint64_t DE      = 1ULL << 3;  // Debugging Extensions
    static constexpr uint64_t PSE     = 1ULL << 4;  // Page Size Extension
    static constexpr uint64_t PAE     = 1ULL << 5;  // Physical Address Extension
    static constexpr uint64_t MCE     = 1ULL << 6;  // Machine Check Enable
    static constexpr uint64_t PGE     = 1ULL << 7;  // Page Global Enable
    static constexpr uint64_t PCE     = 1ULL << 8;  // Performance Monitor Counter Enable
    static constexpr uint64_t OSFXSR  = 1ULL << 9;  // Operating System FXSAVE/FXRSTOR Support
    static constexpr uint64_t OSXMMEX = 1ULL << 10; // OS Support for SSE Exceptions
    static constexpr uint64_t VMXE    = 1ULL << 13; // VMX-Enable Bit
    static constexpr uint64_t SMXE    = 1ULL << 14; // SMX-Enable Bit
    static constexpr uint64_t FSGSBASE= 1ULL << 16; // Enable RDFSBASE/RDGSBASE
    static constexpr uint64_t PCIDE   = 1ULL << 17; // Process-Context Identifiers
    static constexpr uint64_t OSXSAVE = 1ULL << 18; // XSAVE/XRSTOR Enabled
    static constexpr uint64_t SMEP    = 1ULL << 20; // Supervisor Mode Execution Prevention
    static constexpr uint64_t SMAP    = 1ULL << 21; // Supervisor Mode Access Prevention
    static constexpr uint64_t PKE     = 1ULL << 22; // Protection Keys Enable

    static inline uint64_t read() noexcept {
        uint64_t value;
        asm volatile("mov %%cr4, %0" : "=r"(value));
        return value;
    }

    static inline void write(uint64_t value) noexcept {
        asm volatile("mov %0, %%cr4" : : "r"(value));
    }

    // Enable VMX operation by setting VMXE bit
    static inline bool enable_vmx() noexcept {
        uint64_t cr4 = read();
        if (cr4 & VMXE) {
            return true;  // Already enabled
        }
        write(cr4 | VMXE);
        return (read() & VMXE) != 0;
    }
};

// =============================================================================
// Model-Specific Registers (MSRs)
// =============================================================================

template<uint32_t MsrAddr>
struct MSR {
    static inline uint64_t read() noexcept {
        uint32_t low, high;
        asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(MsrAddr));
        return (static_cast<uint64_t>(high) << 32) | low;
    }

    static inline void write(uint64_t value) noexcept {
        uint32_t low = value & 0xFFFFFFFF;
        uint32_t high = value >> 32;
        asm volatile("wrmsr" : : "c"(MsrAddr), "a"(low), "d"(high));
    }
};

// Common MSRs
using IA32_VMX_BASIC            = MSR<0x480>;
using IA32_VMX_PINBASED_CTLS    = MSR<0x481>;
using IA32_VMX_PROCBASED_CTLS   = MSR<0x482>;
using IA32_VMX_EXIT_CTLS        = MSR<0x483>;
using IA32_VMX_ENTRY_CTLS       = MSR<0x484>;
using IA32_FEATURE_CONTROL      = MSR<0x3A>;
using IA32_DEBUGCTL             = MSR<0x1D9>;
using IA32_LSTAR                = MSR<0xC0000082>;  // syscall target RIP
using IA32_CSTAR                = MSR<0xC0000083>;  // sysctl (compat mode)
using IA32_SF_MASK              = MSR<0xC0000084>;  // syscall flag mask
using IA32_KERNEL_GS_BASE       = MSR<0xC0000102>;  // SwapGS base

// =============================================================================
// Control Registers struct for easy manipulation
// =============================================================================

struct ControlRegisters {
    uint64_t cr0;
    uint64_t cr2;
    uint64_t cr3;
    uint64_t cr4;
    uint64_t cr8;

    static inline ControlRegisters capture() noexcept {
        ControlRegisters regs;
        asm volatile(
            "mov %%cr0, %0\n"
            "mov %%cr2, %1\n"
            "mov %%cr3, %2\n"
            "mov %%cr4, %3\n"
            "mov %%cr8, %4\n"
            : "=r"(regs.cr0), "=r"(regs.cr2), "=r"(regs.cr3),
              "=r"(regs.cr4), "=r"(regs.cr8)
        );
        return regs;
    }

    inline void restore() const noexcept {
        asm volatile(
            "mov %0, %%cr0\n"
            "mov %1, %%cr2\n"
            "mov %2, %%cr3\n"
            "mov %3, %%cr4\n"
            "mov %4, %%cr8\n"
            : : "r"(cr0), "r"(cr2), "r"(cr3), "r"(cr4), "r"(cr8)
            : "memory"
        );
    }
};

// =============================================================================
// Segment Descriptor
// =============================================================================

struct [[gnu::packed]] SegmentDescriptor {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  limit_high_flags;
    uint8_t  base_high;

    constexpr SegmentDescriptor() noexcept
        : limit_low(0), base_low(0), base_mid(0), access(0),
          limit_high_flags(0), base_high(0) {}

    void set_base(uint64_t base) noexcept {
        base_low    = base & 0xFFFF;
        base_mid    = (base >> 16) & 0xFF;
        base_high   = (base >> 24) & 0xFF;
    }

    void set_limit(uint32_t limit) noexcept {
        limit_low    = limit & 0xFFFF;
        limit_high_flags = (limit_high_flags & 0xF0) | ((limit >> 16) & 0x0F);
    }

    void set_access(uint8_t access rights) noexcept {
        access = access rights;
    }
};

// =============================================================================
// GDT Entry
// =============================================================================

struct [[gnu::packed]] GDTEntry {
    uint64_t data;

    constexpr GDTEntry() noexcept : data(0) {}

    void set_segment(uint32_t base, uint32_t limit, uint8_t access) noexcept {
        // Build segment descriptor
        uint64_t desc = 0;

        // Base address
        desc |= (base & 0xFFFF) << 16;
        desc |= (base & 0xFF0000) << 16;
        desc |= (base & 0xFF000000) << 32;

        // Limit
        desc |= limit & 0xFFFF;
        desc |= (limit & 0xF0000) << 48;

        // Access byte
        desc |= (uint64_t)access << 40;

        // Flags (granularity, 32-bit)
        desc |= (1ULL << 55) | (1ULL << 54);  // Granularity + 32-bit

        data = desc;
    }
};

// =============================================================================
// RFLAGS Register
// =============================================================================

struct RFLAGS {
    static constexpr uint64_t CF      = 1ULL << 0;   // Carry Flag
    static constexpr uint64_t PF      = 1ULL << 2;   // Parity Flag
    static constexpr uint64_t AF      = 1ULL << 4;   // Auxiliary Carry Flag
    static constexpr uint64_t ZF      = 1ULL << 6;   // Zero Flag
    static constexpr uint64_t SF      = 1ULL << 7;   // Sign Flag
    static constexpr uint64_t TF      = 1ULL << 8;   // Trap Flag
    static constexpr uint64_t IF      = 1ULL << 9;   // Interrupt Enable Flag
    static constexpr uint64_t DF      = 1ULL << 10;  // Direction Flag
    static constexpr uint64_t OF      = 1ULL << 11;  // Overflow Flag
    static constexpr uint64_t NT      = 1ULL << 14;  // Nested Task
    static constexpr uint64_t RF      = 1ULL << 16;  // Resume Flag
    static constexpr uint64_t VM      = 1ULL << 17;  // Virtual 8086 Mode
    static constexpr uint64_t AC      = 1ULL << 18;  // Alignment Check
    static constexpr uint64_t VIF     = 1ULL << 19;  // Virtual Interrupt Flag
    static constexpr uint64_t VIP     = 1ULL << 20;  // Virtual Interrupt Pending
    static constexpr uint64_t ID      = 1ULL << 21;  // ID Flag

    static inline uint64_t read() noexcept {
        uint64_t flags;
        asm volatile("pushfq\n pop %0" : "=r"(flags));
        return flags;
    }

    static inline void write(uint64_t flags) noexcept {
        asm volatile("push %0\n popfq" : : "r"(flags) : "cc");
    }
};

// =============================================================================
// CPUID Feature Detection
// =============================================================================

struct CPUID {
    struct [[gnu::packed]] Features {
        bool eax_mmx    : 1;
        bool eax_sse    : 1;
        bool eax_sse2   : 1;
        bool ecx_sse3   : 1;
        bool ecx_ssse3  : 1;
        bool ecx_sse4_1 : 1;
        bool ecx_sse4_2 : 1;
        bool ecx_avx    : 1;
        bool ecx_avx2   : 1;
        bool ecx_avx512 : 1;
        bool ecx_vmx    : 1;
        bool ecx_rdrand : 1;
    };

    static inline Features get_features() noexcept {
        Features features{};
        uint32_t eax, ebx, ecx, edx;

        // CPUID leaf 1 - Processor Info and Feature Bits
        asm volatile("cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(1)
        );

        features.eax_sse    = (edx >> 25) & 1;
        features.eax_sse2   = (edx >> 26) & 1;
        features.ecx_sse3   = (ecx) & 1;
        features.ecx_ssse3  = (ecx >> 9) & 1;
        features.ecx_sse4_1 = (ecx >> 19) & 1;
        features.ecx_sse4_2 = (ecx >> 20) & 1;
        features.ecx_avx    = (ecx >> 28) & 1;
        features.ecx_vmx    = (ecx >> 5) & 1;
        features.ecx_rdrand = (ecx >> 30) & 1;

        // CPUID leaf 7 - Extended Features
        asm volatile("cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(7), "c"(0)
        );

        features.ecx_avx2 = (ebx >> 5) & 1;
        features.ecx_avx512 = (ebx >> 16) & 1;

        return features;
    }
};

} // namespace regs
} // namespace hypervisor

#endif // REGISTERS_HPP
