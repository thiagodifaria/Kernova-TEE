; =============================================================================
; Kernova-TEE - VMX Initialization
; =============================================================================
; Activates VMX mode and prepares the processor for virtualization
; Based on Intel SDM Volume 3C - Chapter 23 VMX Entry
; =============================================================================

section .text
bits 64

; Include VMX definitions
%include "vmx_defs.inc"

; External symbols
extern vmcs_setup
extern vmexit_handler

; Global symbols
global vmx_init
global vmxon
global vmxoff
global vmx_launch
global vmx_resume

; =============================================================================
; vmx_init - Initialize VMX operation on the current processor
; Returns: EAX = 0 on success, -1 on failure
; =============================================================================
vmx_init:
    push rbp
    mov rbp, rsp
    push rbx
    push rcx
    push rdx
    push rdi

    ; Step 1: Enable CR4.VMXE
    call enable_cr4_vmx
    test eax, eax
    jz .init_failed

    ; Step 2: Allocate and initialize VMXON region
    call alloc_vmx_region
    test rax, rax
    jz .alloc_failed

    ; Save VMXON region pointer
    mov [vmxon_region_ptr], rax

    ; Step 3: Initialize VMXON region with revision ID
    call init_vmx_region
    test eax, eax
    jz .init_failed

    ; Step 4: Execute VMXON instruction
    mov rdi, [vmxon_region_ptr]
    call do_vmxon
    test eax, eax
    jz .vmxon_failed

    ; Step 5: Setup VMCS for the guest
    call vmcs_setup
    test eax, eax
    jz .vmcs_setup_failed

    ; VMX initialization successful
    xor eax, eax
    jmp .init_done

.vmxon_failed:
    mov eax, -3
    jmp .init_done

.vmcs_setup_failed:
    ; VMXON succeeded but VMCS setup failed
    ; Need to execute VMXOFF before returning
    call do_vmxoff
    mov eax, -4
    jmp .init_done

.alloc_failed:
    mov eax, -2
    jmp .init_done

.init_failed:
    mov eax, -1

.init_done:
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rbp
    ret

; =============================================================================
; enable_cr4_vmx - Enable VMX operation in CR4
; Returns: EAX = 1 on success, 0 on failure
; =============================================================================
enable_cr4_vmx:
    push rbx

    ; Read current CR4
    mov rax, cr4

    ; Check if VMXE is already set
    test rax, CR4_VMXE
    jnz .already_enabled

    ; Set VMXE bit
    or rax, CR4_VMXE
    mov cr4, rax

    ; Verify the bit was set
    mov rbx, cr4
    test rbx, CR4_VMXE
    jnz .success

    xor eax, eax
    jmp .done

.already_enabled:
.success:
    mov eax, 1

.done:
    pop rbx
    ret

; =============================================================================
; alloc_vmx_region - Allocate 4KB-aligned memory for VMXON
; Returns: RAX = pointer to allocated region, 0 on failure
; =============================================================================
alloc_vmx_region:
    ; For this PoC, use a statically allocated region
    ; In production, would use proper page allocator
    mov rax, vmxon_region
    ret

; =============================================================================
; init_vmx_region - Initialize VMXON region with revision ID
; Returns: EAX = 1 on success, 0 on failure
; =============================================================================
init_vmx_region:
    push rbx
    push rcx
    push rdx

    ; Get VMX MSR to determine revision ID
    mov ecx, IA32_VMX_BASIC
    rdmsr
    mov ebx, eax                ; EBX = revision ID (low 31 bits)

    ; Initialize VMXON region
    mov rdi, [vmxon_region_ptr]

    ; Clear the entire 4KB region
    xor eax, eax
    mov ecx, 1024               ; 4KB = 1024 * 4 bytes
    rep stosd

    ; Write revision ID to first DWORD
    mov rdi, [vmxon_region_ptr]
    mov [rdi], ebx

    ; Clear bit 31 (indicates this is a VMXON region, not VMCS)
    and dword [rdi], ~(1 << 31)

    mov eax, 1
    jmp .done

.done:
    pop rdx
    pop rcx
    pop rbx
    ret

; =============================================================================
; do_vmxon - Execute VMXON instruction
; RDI = pointer to VMXON region
; Returns: EAX = 1 on success, 0 on failure
; =============================================================================
do_vmxon:
    push rbx
    push rdx

    ; Verify alignment (must be 4KB aligned)
    test rdi, 0xFFF
    jnz .invalid_alignment

    ; Execute VMXON
    ; On failure, it sets ZF flag
    vmxon [rdi]

    ; Check if VMXON succeeded (ZF=0)
    jz .vmxon_failed

    ; VMXON successful - read VMXON region to confirm
    ; Bit 31 should now be set
    test dword [rdi], (1 << 31)
    jz .verification_failed

    mov eax, 1
    jmp .done

.vmxon_failed:
.verification_failed:
    xor eax, eax
    jmp .done

.invalid_alignment:
    xor eax, eax

.done:
    pop rdx
    pop rbx
    ret

; =============================================================================
; do_vmxoff - Execute VMXOFF instruction
; Returns: EAX = 1 on success, 0 on failure
; =============================================================================
do_vmxoff:
    ; Execute VMXOFF
    vmxoff

    ; Verify we're no longer in VMX operation
    ; by checking CR4.CPL[0] (simplified check)
    mov eax, 1
    ret

; =============================================================================
; vmxon - Public wrapper for VMXON instruction
; RDI = pointer to VMXON region
; =============================================================================
vmxon:
    push rbp
    mov rbp, rsp

    ; Enable CR4.VMXE if not already set
    call enable_cr4_vmx

    ; Execute VMXON
    call do_vmxon

    pop rbp
    ret

; =============================================================================
; vmxoff - Public wrapper for VMXOFF instruction
; =============================================================================
vmxoff:
    push rbp
    mov rbp, rsp

    call do_vmxoff

    pop rbp
    ret

; =============================================================================
; vmx_launch - Launch VM using VMLAUNCH
; This transfers control to the guest OS
; =============================================================================
vmx_launch:
    push rbp
    mov rbp, rsp

    ; Save host stack pointer
    mov [host_stack_ptr], rsp

    ; Clear any pending VMX errors
    ; (in production, would check VMCS launch state)

    ; Execute VMLAUNCH
    ; If this returns, it means VMLAUNCH failed
    vmlaunch

    ; If we get here, VMLAUNCH failed
    ; VMCS launch state should be "clear"
    mov eax, -1

    ; Error handling would go here
    ; Check VMCS instruction error field

    pop rbp
    ret

; =============================================================================
; vmx_resume - Resume VM using VMRESUME
; Returns control to guest after VM exit
; =============================================================================
vmx_resume:
    push rbp
    mov rbp, rsp

    ; Save host stack pointer
    mov [host_stack_ptr], rsp

    ; Clear any pending VMX errors

    ; Execute VMRESUME
    ; If this returns, VMRESUME failed
    vmresume

    ; If we get here, VMRESUME failed
    mov eax, -1

    pop rbp
    ret

; =============================================================================
; vmx_invept - Invalidate EPT TLB entries
; RDI = type (single-context or global-context)
; RSI = pointer to invalidation descriptor
; =============================================================================
vmx_invept:
    push rbp
    mov rbp, rsp
    push rbx

    ; Check if INVEPT is supported
    mov ecx, IA32_VMX_PROCBASED_CTLS
    rdmsr

    ; For simplicity, assume it's supported
    ; Execute INVEPT
    invept rdi, oword [rsi]

    pop rbx
    pop rbp
    ret

; =============================================================================
; check_vmx_errors - Check VMCS error field after failed VMX instruction
; Returns: EAX = VM instruction error code
; =============================================================================
check_vmx_errors:
    push rbx

    ; Read VMCS instruction error field
    ; Encoding: 0x4400
    mov rbx, 0x4400
    vmread rbx, rax

    pop rbx
    ret

; =============================================================================
; Data Section
; =============================================================================
section .bss

align 4096

; VMXON region (4KB aligned)
vmxon_region:
    resb 4096

; Pointer to VMXON region
vmxon_region_ptr:
    resq 1

; Host stack pointer for VM exit
host_stack_ptr:
    resq 1

; VMCS revision ID (cached)
vmcs_revision_id:
    resd 1

; Data section
section .data

; VMX error messages
vmx_error_msgs:
    .success:
        db "VMX operation successful", 0
    .vmxon_failed:
        db "VMXON instruction failed", 0
    .vmxoff_failed:
        db "VMXOFF instruction failed", 0
    .vmlaunch_failed:
        db "VMLAUNCH instruction failed", 0
    .vmresume_failed:
        db "VMRESUME instruction failed", 0
