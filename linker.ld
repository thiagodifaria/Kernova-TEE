/* =============================================================================
 * Micro-Hypervisor Linker Script
 * =============================================================================
 * Defines memory layout for the bare-metal hypervisor
 * ============================================================================= */

ENTRY(_start)

/* Define hypervisor memory segments */
SECTIONS
{
    /* Multiboot header must be in the first 8KB */
    . = 0x100000;  /* Load at 1MB (standard for bootloaders) */

    .boot : {
        *(multiboot_header)
        *(.text.boot)
    }

    /* Text section (code) */
    .text ALIGN(4K) : {
        *(.text*)
        *(.rodata*)
    }

    /* Read-only data */
    .rodata ALIGN(4K) : {
        *(.rodata*)
    }

    /* Initialized data */
    .data ALIGN(4K) : {
        *(.data*)
    }

    /* Uninitialized data */
    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss*)
    }

    /* VMX regions (must be 4KB aligned) */
    .vmx ALIGN(4K) : {
        *(.vmx*)
    } = 0

    /* Stack */
    .stack ALIGN(16) : {
        *(.stack*)
        . = ALIGN(16);
        *(.stack*)
        . += 0x4000;  /* 16KB stack */
        _stack_top = .;
    }

    /* Enclave memory region */
    .enclave ALIGN(4K) : {
        _enclave_start = .;
        *(.enclave*)
        . = ALIGN(4K);
        _enclave_end = .;
    } = 0

    /* Discard unwanted sections */
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame*)
        *(.gcc_except_table*)
    }

    /* Provide symbols for C++ code */
    _hypervisor_end = .;
}

/* Provide stack symbols */
PROVIDE(_stack_pointer = _stack_top);
PROVIDE(_enclave_size = _enclave_end - _enclave_start);
