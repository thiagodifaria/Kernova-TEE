; =============================================================================
; Micro-Hypervisor - VM Exit Handler & Rootkit Detector
; =============================================================================
; Intercepts sensitive operations and detects rootkit behavior
; Monitors CR access, MSR writes, and syscall interception attempts
; =============================================================================

section .text
bits 64

; Include VMX definitions
%include "../../include/vmx_defs.inc"

; External symbols
extern trace_log_event
extern trace_check_integrity

; Global symbols
global vmexit_handler
global vmexit_dispatch
global intercept_cr_access
global intercept_msr_write
global intercept_cpuid
global intercept_io_instruction

; =============================================================================
; vmexit_handler - Main VM Exit Handler
; Called by processor on any VM exit
; =============================================================================
vmexit_handler:
    ; Save all general purpose registers
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

    ; Save segment registers
    mov ax, ds
    push rax
    mov ax, es
    push rax
    mov ax, fs
    push rax
    mov ax, gs
    push rax

    ; Load hypervisor data segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Read exit reason from VMCS
    mov rdi, VMCS_EXIT_REASON
    call vmcs_read64
    mov r15, rax                ; Save exit reason

    ; Read exit qualification (if needed)
    mov rdi, VMCS_EXIT_QUALIFICATION
    call vmcs_read64
    mov r14, rax                ; Save qualification

    ; Read guest RIP (instruction that caused exit)
    mov rdi, VMCS_GUEST_RIP
    call vmcs_read64
    mov r13, rax                ; Save guest RIP

    ; Read instruction length
    mov rdi, VMCS_EXIT_INSTR_LEN
    call vmcs_read32
    mov r12d, eax               ; Save instruction length

    ; Dispatch to specific handler
    mov rdi, r15                ; exit reason
    call vmexit_dispatch

    ; Update guest RIP to skip the instruction that caused exit
    mov rdi, VMCS_GUEST_RIP
    mov rsi, r13
    add rsi, r12
    call vmcs_write64

    ; Restore segment registers
    pop rax
    mov gs, ax
    pop rax
    mov fs, ax
    pop rax
    mov es, ax
    pop rax
    mov ds, ax

    ; Restore general purpose registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; Resume guest execution
    vmresume

    ; If VMRESUME fails, handle error
    jmp vmexit_error

; =============================================================================
; vmexit_dispatch - Dispatch VM exit to specific handler
; RDI = exit reason
; =============================================================================
vmexit_dispatch:
    push rbp
    mov rbp, rsp

    ; Switch on exit reason
    cmp rdi, EXIT_REASON_CR_ACCESS
    je .handle_cr_access

    cmp rdi, EXIT_REASON_MSR_WRITE
    je .handle_msr_write

    cmp rdi, EXIT_REASON_CPUID
    je .handle_cpuid

    cmp rdi, EXIT_REASON_IO_INSTR
    je .handle_io

    cmp rdi, EXIT_REASON_VMCALL
    je .handle_vmcall

    cmp rdi, EXIT_REASON_HLT
    je .handle_hlt

    ; Default: unknown exit reason
    jmp .exit_done

.handle_cr_access:
    call intercept_cr_access
    jmp .exit_done

.handle_msr_write:
    call intercept_msr_write
    jmp .exit_done

.handle_cpuid:
    call intercept_cpuid
    jmp .exit_done

.handle_io:
    call intercept_io_instruction
    jmp .exit_done

.handle_vmcall:
    ; VMCALL - guest request to hypervisor
    xor eax, eax                ; Return success
    jmp .exit_done

.handle_hlt:
    ; HLT instruction - just resume
    xor eax, eax
    jmp .exit_done

.exit_done:
    pop rbp
    ret

; =============================================================================
; intercept_cr_access - Detect CR register modification (rootkit technique)
; Rootkits often modify CR0.WP to disable write protection
; =============================================================================
intercept_cr_access:
    push rbp
    mov rbp, rsp
    push rbx
    push rdi

    ; R14 = exit qualification
    mov rax, r14

    ; Parse qualification:
    ; Bits [3:0] - CR number (0-3)
    ; Bits [5:4] - Access type (0=mov to CR, 1=mov from CR, 2=CLTS, 3=LMSW)
    ; Bits [7:6] - LMSW type
    ; Bits [12:8] - GPR register
    ; Bits [63:16] - Reserved

    mov rbx, rax
    and rbx, 0xF                ; CR number

    cmp rbx, 0
    je .check_cr0

    cmp rbx, 4
    je .check_cr4

    ; Other CRs - allow
    jmp .allow_access

.check_cr0:
    ; CR0 access - check for WP bit modification
    ; CR0.WP (Write Protect) at bit 16
    ; Rootkits try to clear this to modify kernel code

    mov rdi, VMCS_EXIT_QUALIFICATION
    call vmcs_read64
    mov rbx, rax

    and rax, 0x30               ; Access type in bits [5:4]
    shr rax, 4
    cmp rax, 0
    jne .allow_access           ; Not a write, allow

    ; This is a CR write - log it
    mov rdi, EVENT_CR0_WRITE
    mov rsi, rbx
    call trace_log_event

    ; Check if WP bit is being cleared
    ; (We'd need to read the new value from GPR)
    ; For PoC, just log the event

    jmp .allow_access

.check_cr4:
    ; CR4 access - check for suspicious modifications
    ; CR4.VMXE, CR4.SMEP, CR4.SMAP are security-critical

    mov rdi, EVENT_CR4_WRITE
    mov rsi, rax
    call trace_log_event

    jmp .allow_access

.allow_access:
    ; Inject the original instruction or skip it
    ; For now, just skip by updating RIP
    xor eax, eax
    jmp .done

.done:
    pop rdi
    pop rbx
    pop rbp
    ret

; =============================================================================
; intercept_msr_write - Detect MSR writes (syscalls hooks)
; Rootkits hook IA32_LSTAR to intercept syscalls
; =============================================================================
intercept_msr_write:
    push rbp
    mov rbp, rsp
    push rbx
    push rdi
    push rsi

    ; Exit qualification contains MSR number in RCX
    ; We need to check which MSR is being written

    ; Read the MSR number from guest RCX
    ; This is complex - need to read from VMCS guest state
    ; For PoC, we'll use a simpler approach

    ; Check if this is IA32_LSTAR (syscall target)
    mov rdi, VMCS_GUEST_RCX
    call vmcs_read64
    mov rbx, rax                ; RBX = MSR address

    cmp rax, 0xC0000082         ; IA32_LSTAR
    je .check_lstar_hook

    cmp rax, 0xC0000081         ; IA32_STAR
    je .check_star_hook

    cmp rax, 0xC0000084         ; IA32_SF_MASK
    je .check_sfmask_hook

    ; Other MSRs - allow
    jmp .allow_msr

.check_lstar_hook:
    ; IA32_LSTAR is being written
    ; This is how rootkits hook syscalls

    ; Log the event
    mov rdi, EVENT_LSTAR_WRITE
    mov rsi, rbx
    call trace_log_event

    ; Check integrity of syscall handler
    call trace_check_integrity

    jmp .allow_msr

.check_star_hook:
    mov rdi, EVENT_STAR_WRITE
    call trace_log_event
    jmp .allow_msr

.check_sfmask_hook:
    mov rdi, EVENT_SFMASK_WRITE
    call trace_log_event
    jmp .allow_msr

.allow_msr:
    ; Allow the MSR write
    xor eax, eax
    jmp .done

.done:
    pop rsi
    pop rdi
    pop rbx
    pop rbp
    ret

; =============================================================================
; intercept_cpuid - Handle CPUID instructions
; Can be used for hypervisor detection
; =============================================================================
intercept_cpuid:
    push rbp
    mov rbp, rsp
    push rbx
    push rcx
    push rdx

    ; Read guest RAX (CPUID leaf)
    mov rdi, VMCS_GUEST_RAX
    call vmcs_read64
    mov rbx, rax                ; RBX = leaf

    ; Read guest RCX (sub-leaf)
    mov rdi, VMCS_GUEST_RCX
    call vmcs_read64
    mov rcx, rax

    ; Check for hypervisor detection attempts
    cmp rbx, 1
    jne .not_hypervisor_check

    ; CPUID.1:ECX[bit 31] = hypervisor present
    ; We can hide ourselves by not setting this bit
    ; Or we can be transparent

    ; Execute the CPUID
    cpuid

    ; Store results in VMCS guest registers
    mov rdi, VMCS_GUEST_RAX
    mov rsi, rax
    call vmcs_write64

    mov rdi, VMCS_GUEST_RBX
    mov rsi, rbx
    call vmcs_write64

    mov rdi, VMCS_GUEST_RCX
    mov rsi, rcx
    call vmcs_write64

    mov rdi, VMCS_GUEST_RDX
    mov rsi, rdx
    call vmcs_write64

    jmp .done

.not_hypervisor_check:
    ; Handle other CPUID requests
    ; For PoC, just execute and return results
    cpuid

    mov rdi, VMCS_GUEST_RAX
    mov rsi, rax
    call vmcs_write64

    mov rdi, VMCS_GUEST_RBX
    mov rsi, rbx
    call vmcs_write64

    mov rdi, VMCS_GUEST_RCX
    mov rsi, rcx
    call vmcs_write64

    mov rdi, VMCS_GUEST_RDX
    mov rsi, rdx
    call vmcs_write64

.done:
    pop rdx
    pop rcx
    pop rbx
    pop rbp
    ret

; =============================================================================
; intercept_io_instruction - Handle I/O instructions (IN/OUT)
; =============================================================================
intercept_io_instruction:
    push rbp
    mov rbp, rsp

    ; Parse exit qualification for I/O details
    ; For PoC, just allow the I/O

    xor eax, eax
    pop rbp
    ret

; =============================================================================
; vmexit_error - Handle VMRESUME failure
; =============================================================================
vmexit_error:
    ; VMRESUME failed - critical error
    cli
.error_halt:
    hlt
    jmp .error_halt

; =============================================================================
; Helper Functions
; =============================================================================

; Read 64-bit field from VMCS
; RDI = field encoding
; Returns: RAX = value
vmcs_read64:
    vmread rdi, rax
    ret

; Write 64-bit field to VMCS
; RDI = field encoding
; RSI = value
vmcs_write64:
    vmwrite rdi, rsi
    ret

; Read 32-bit field from VMCS
; RDI = field encoding
; Returns: EAX = value
vmcs_read32:
    vmread rdi, rax
    ret

; =============================================================================
; Event Types for Logging
; =============================================================================
section .rodata

EVENT_CR0_WRITE:
    db "CR0 write detected", 0

EVENT_CR4_WRITE:
    db "CR4 write detected", 0

EVENT_LSTAR_WRITE:
    db "IA32_LSTAR write (syscall hook attempt)", 0

EVENT_STAR_WRITE:
    db "IA32_STAR write", 0

EVENT_SFMASK_WRITE:
    db "IA32_SF_MASK write", 0

EVENT suspicious_msr:
    db "Suspicious MSR write", 0

; =============================================================================
; Statistics
; =============================================================================
section .bss

align 8

; VM Exit statistics
vmexit_stats:
    .total_exits:
        resq 1
    .cr_access_count:
        resq 1
    .msr_write_count:
        resq 1
    .cpuid_count:
        resq 1
    .io_count:
        resq 1
    .suspicious_events:
        resq 1
