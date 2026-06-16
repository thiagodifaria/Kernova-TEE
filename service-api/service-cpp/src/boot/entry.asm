; =============================================================================
; Kernova-TEE - Boot Entry Point
; =============================================================================
; System entry and transition to Long Mode with VMX initialization
; =============================================================================

section .text
bits 64

; External C++ functions
extern hypervisor_init
extern hypervisor_main

; External VMX functions
extern vmx_init
extern vmcs_setup

; =============================================================================
; Multiboot2 Header (for QEMU/Legacy boot support)
; =============================================================================
align 8
multiboot_header:
    dd 0xE85250D6                  ; Magic number
    dd 0                           ; Architecture (i386 = 0, x86-64 would need different)
    dd multiboot_header_end - multiboot_header  ; Header length
    dd -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header))  ; Checksum

    ; End tag
    dw 0, 0
    dd 8
multiboot_header_end:

; =============================================================================
; Entry Point - Called by bootloader or directly
; =============================================================================
global _start
_start:
    ; Disable interrupts
    cli

    ; Save stack pointer
    mov [boot_stack_ptr], rsp

    ; Clear segment registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Setup new stack
    mov rsp, stack_top

    ; Check if we're already in Long Mode
    call check_long_mode
    test eax, eax
    jnz .in_long_mode

    ; Transition to Long Mode (if in protected mode)
    call enable_long_mode

.in_long_mode:
    ; Verify we can use VMX instructions
    call check_vmx_support
    test eax, eax
    jz .vmx_not_supported

    ; Setup identity-mapped page tables for VMX
    call setup_page_tables

    ; Initialize FPU and SIMD state
    call init_simd_state

    ; Call C++ initialization
    call hypervisor_init
    test eax, eax
    jnz .init_failed

    ; Jump to main hypervisor loop
    call hypervisor_main

    ; Should never return
.hypervisor_panic:
    cli
.hypervisor_halt:
    hlt
    jmp .hypervisor_halt

.vmx_not_supported:
    ; VMX not supported - cannot continue
    mov rdi, VMX_ERROR_MSG
    call print_error
    jmp .hypervisor_halt

.init_failed:
    mov rdi, INIT_ERROR_MSG
    call print_error
    jmp .hypervisor_halt

; =============================================================================
; check_long_mode - Verify CPU is in Long Mode (64-bit mode)
; Returns: EAX = 1 if in Long Mode, 0 otherwise
; =============================================================================
check_long_mode:
    push rbx

    ; Read EFER_MSR (Extended Feature Enable Register)
    mov ecx, 0xC0000080       ; IA32_EFER MSR
    rdmsr
    test eax, (1 << 10)       ; Check LMA bit (bit 10)
    jnz .is_long_mode

    xor eax, eax
    jmp .check_done

.is_long_mode:
    mov eax, 1

.check_done:
    pop rbx
    ret

; =============================================================================
; enable_long_mode - Transition from Protected to Long Mode
; =============================================================================
enable_long_mode:
    ; This is a simplified version
    ; In real implementation, would need to:
    ; 1. Check CPUID for long mode support
    ; 2. Setup page tables
    ; 3. Set CR4.PAE
    ; 4. Set EFER.LME
    ; 5. Set CR0.PG
    ; For this PoC, we assume we're loaded into 64-bit mode

    ret

; =============================================================================
; check_vmx_support - Verify CPU supports Intel VT-x
; Returns: EAX = 1 if VMX supported, 0 otherwise
; =============================================================================
check_vmx_support:
    push rbx
    push rcx
    push rdx

    ; Check CPUID for VMX support
    mov eax, 1
    cpuid
    test ecx, (1 << 5)        ; Check VMX bit in CPUID.1:ECX
    jz .not_supported

    ; Check if VMX is locked and enabled in IA32_FEATURE_CONTROL
    mov ecx, 0x3A             ; IA32_FEATURE_CONTROL MSR
    rdmsr

    test eax, 1               ; Check lock bit
    jz .not_locked

    test eax, (1 << 2)        ; Check VMX outside SMX bit
    jz .not_supported

    ; VMX is supported and enabled
    mov eax, 1
    jmp .vmx_check_done

.not_locked:
    ; Try to enable VMX
    or eax, (1 << 2)          ; Set VMX outside SMX
    or eax, 1                 ; Set lock bit
    wrmsr

    mov eax, 1
    jmp .vmx_check_done

.not_supported:
    xor eax, eax

.vmx_check_done:
    pop rdx
    pop rcx
    pop rbx
    ret

; =============================================================================
; setup_page_tables - Identity-mapped pages for VMX operation
; =============================================================================
setup_page_tables:
    ; VMX requires identity-mapped pages for VMXON region and VMCS
    ; This is a simplified implementation

    ; In a real implementation, would:
    ; 1. Allocate page tables
    ; 2. Map the required regions 1:1
    ; 3. Set CR3 to point to PML4

    ret

; =============================================================================
; init_simd_state - Initialize SSE/AVX state
; =============================================================================
init_simd_state:
    ; Initialize SIMD state
    vzeroupper                ; Clear YMM registers

    ; Set up MXCSR for SSE operations
    ldmxcsr [simd_mxcsr_init]

    ; Enable SSE and AVX in OSFXSR
    mov rax, cr4
    or rax, (1 << 9)          ; OSFXSR bit
    or rax, (1 << 10)         ; OSXMMEXCPT bit
    mov cr4, rax

    ret

; =============================================================================
; print_error - Simple error output (for debugging)
; RDI = pointer to error message string
; =============================================================================
print_error:
    ; This would write to serial port or VGA text buffer
    ; For PoC, just halt
    ret

; =============================================================================
; Exception Handlers
; =============================================================================
global exception_handler
exception_handler:
    ; Save all registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov ax, ds
    push rax
    mov ax, es
    push rax
    mov ax, fs
    push rax
    mov ax, gs
    push rax

    ; TODO: Log exception and attempt recovery

    ; Halt for now
    cli
.exception_halt:
    hlt
    jmp .exception_halt

; =============================================================================
; Global Descriptor Table (GDT) for 64-bit mode
; =============================================================================
section .rodata
align 16

gdt64:
    .null: equ $ - gdt64
    dq 0                        ; Null descriptor

    .code: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; Code segment

    .data: equ $ - gdt64
    dq (1 << 41) | (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; Data segment

gdt64_ptr:
    dw gdt64_end - gdt64 - 1    ; Limit
    dq gdt64                    ; Base
gdt64_end:

; =============================================================================
; BSS Section - Data and Stack
; =============================================================================
section .bss

align 16

boot_stack_ptr:
    resq 1

; Stack for the hypervisor
stack_bottom:
    resb 16384                  ; 16KB stack
stack_top:

; SIMD control register initial value (MXCSR)
simd_mxcsr_init:
    resd 1                      ; Will be initialized to 0x1F80

; =============================================================================
; Data Section
; =============================================================================
section .data

VMX_ERROR_MSG:
    db "ERROR: VMX not supported or disabled in BIOS", 0

INIT_ERROR_MSG:
    db "ERROR: Hypervisor initialization failed", 0

; =============================================================================
; Exported symbols for C++ code
; =============================================================================
global get_gdt_ptr
get_gdt_ptr:
    mov rax, gdt64_ptr
    ret
