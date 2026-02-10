; =============================================================================
; Kernova-TEE - VMCS Configuration
; =============================================================================
; Configures the Virtual Machine Control Structure for guest operation
; Based on Intel SDM Volume 3C - Chapter 24 VMCS Fields
; =============================================================================

section .text
bits 64

; Include VMX definitions
%include "vmx_defs.inc"

; External symbols
extern vmexit_handler

; Global symbols
global vmcs_setup
global vmcs_load
global vmcs_clear
global vmcs_write
global vmcs_read
global vmcs_launch_state

; =============================================================================
; vmcs_setup - Setup and configure VMCS for guest operation
; Returns: EAX = 0 on success, -1 on failure
; =============================================================================
vmcs_setup:
    push rbp
    mov rbp, rsp
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi

    ; Step 1: Initialize VMCS region
    call alloc_vmcs_region
    test rax, rax
    jz .alloc_failed

    mov [vmcs_region_ptr], rax

    ; Step 2: Clear VMCS (mark it inactive)
    mov rdi, rax
    call do_vmclear
    test eax, eax
    jz .clear_failed

    ; Step 3: Load VMCS pointer
    mov rdi, [vmcs_region_ptr]
    call do_vmptrld
    test eax, eax
    jz .load_failed

    ; Step 4: Configure VMCS fields
    call configure_vmcs_control
    test eax, eax
    jz .config_failed

    call configure_vmcs_guest
    test eax, eax
    jz .config_failed

    call configure_vmcs_host
    test eax, eax
    jz .config_failed

    ; VMCS setup successful
    xor eax, eax
    jmp .setup_done

.config_failed:
    mov eax, -4
    jmp .setup_done

.load_failed:
    mov eax, -3
    jmp .setup_done

.clear_failed:
    mov eax, -2
    jmp .setup_done

.alloc_failed:
    mov eax, -1

.setup_done:
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rbp
    ret

; =============================================================================
; alloc_vmcs_region - Allocate 4KB-aligned memory for VMCS
; Returns: RAX = pointer to VMCS region
; =============================================================================
alloc_vmcs_region:
    mov rax, vmcs_region
    ret

; =============================================================================
; do_vmclear - Clear VMCS (mark inactive)
; RDI = pointer to VMCS region
; Returns: EAX = 1 on success, 0 on failure
; =============================================================================
do_vmclear:
    push rbx

    ; Verify 4KB alignment
    test rdi, 0xFFF
    jnz .invalid_alignment

    ; Execute VMCLEAR
    vmclear [rdi]

    ; Check for success (ZF=0)
    jz .clear_failed

    mov eax, 1
    jmp .done

.clear_failed:
.invalid_alignment:
    xor eax, eax

.done:
    pop rbx
    ret

; =============================================================================
; do_vmptrld - Load VMCS pointer (make current VMCS active)
; RDI = pointer to VMCS region
; Returns: EAX = 1 on success, 0 on failure
; =============================================================================
do_vmptrld:
    push rbx

    ; Verify 4KB alignment
    test rdi, 0xFFF
    jnz .invalid_alignment

    ; Execute VMPTRLD
    vmptrld [rdi]

    ; Check for success (ZF=0)
    jz .load_failed

    mov eax, 1
    jmp .done

.load_failed:
.invalid_alignment:
    xor eax, eax

.done:
    pop rbx
    ret

; =============================================================================
; configure_vmcs_control - Setup VMCS control fields
; Returns: EAX = 1 on success, 0 on failure
; =============================================================================
configure_vmcs_control:
    push rbx
    push rcx
    push rdx

    ; Get VMX MSR values for default controls
    ; Pin-based controls
    mov ecx, IA32_VMX_PINBASED_CTLS
    rdmsr
    ; EAX = 0s that must be set, EBX = 1s that must be clear
    ; Use the defaults for now
    mov edx, VMCS_CTRL_16BIT | 0x0000  ; Pin-based controls offset

    ; Configure: External interrupts exiting + NMI exiting
    mov ebx, eax
    or ebx, (PIN_BASED_EXT_INTR_EXIT | PIN_BASED_NMI_EXIT)
    and ebx, edx

    ; Write to VMCS
    mov rcx, (VMCS_CTRL_16BIT | 0x0000)
    vmwrite rcx, rbx

    ; Primary processor-based VM-execution controls
    mov ecx, IA32_VMX_PROCBASED_CTLS
    rdmsr

    mov rbx, rax
    ; Enable secondary controls, HLT exiting, CR3 load/store
    mov r8, (PROC_BASED_HLT_EXIT | PROC_BASED_CR3_LOAD | PROC_BASED_CR3_STORE | PROC_BASED_SECONDARY_CTLS)
    or rbx, r8
    mov rcx, (VMCS_CTRL_32BIT | 0x4002)
    vmwrite rcx, rbx

    ; VM-exit controls
    mov ecx, IA32_VMX_EXIT_CTLS
    rdmsr

    mov rbx, rax
    ; Enable 64-bit host
    or rbx, EXIT_CONTROLS_HOST_ADDR64
    mov rcx, (VMCS_CTRL_32BIT | 0x400C)
    vmwrite rcx, rbx

    ; VM-entry controls
    mov ecx, IA32_VMX_ENTRY_CTLS
    rdmsr

    mov rbx, rax
    ; IA-32e mode guest
    or rbx, ENTRY_CONTROLS_IA64_MODE
    mov rcx, (VMCS_CTRL_32BIT | 0x4012)
    vmwrite rcx, rbx

    mov eax, 1
    jmp .done

.done:
    pop rdx
    pop rcx
    pop rbx
    ret

; =============================================================================
; configure_vmcs_guest - Setup guest state fields
; Returns: EAX = 1 on success, 0 on failure
; =============================================================================
configure_vmcs_guest:
    push rbx
    push rcx

    ; Guest segment selectors
    ; CS: Code segment (selector 0x08 = second GDT entry)
    mov rbx, 0x08
    mov rcx, VMCS_GUEST_CS_SELECTOR
    vmwrite rcx, rbx

    ; DS, ES, SS, FS, GS: Data segments (selector 0x10 = third GDT entry)
    mov rbx, 0x10
    mov rcx, VMCS_GUEST_DS_SELECTOR
    vmwrite rcx, rbx
    mov rcx, VMCS_GUEST_ES_SELECTOR
    vmwrite rcx, rbx
    mov rcx, VMCS_GUEST_SS_SELECTOR
    vmwrite rcx, rbx
    mov rcx, VMCS_GUEST_FS_SELECTOR
    vmwrite rcx, rbx
    mov rcx, VMCS_GUEST_GS_SELECTOR
    vmwrite rcx, rbx

    ; Guest segment limits (4GB)
    mov rbx, 0xFFFFFFFF
    mov rcx, VMCS_GUEST_CS_LIMIT
    vmwrite rcx, rbx
    mov rcx, VMCS_GUEST_DS_LIMIT
    vmwrite rcx, rbx
    mov rcx, VMCS_GUEST_ES_LIMIT
    vmwrite rcx, rbx
    mov rcx, VMCS_GUEST_SS_LIMIT
    vmwrite rcx, rbx

    ; Guest control registers
    ; CR0: PE (Protection Enable) + other necessary bits
    mov rbx, CR0_PE
    mov rcx, VMCS_GUEST_CR0
    vmwrite rcx, rbx

    ; CR3: Page table base (for now, identity mapped)
    mov rbx, 0
    mov rcx, VMCS_GUEST_CR3
    vmwrite rcx, rbx

    ; CR4: PAE + other bits
    mov rbx, (1 << 5)  ; PAE bit
    mov rcx, VMCS_GUEST_CR4
    vmwrite rcx, rbx

    ; Guest RIP and RSP (where guest starts execution)
    ; For this PoC, point to a safe location
    mov rbx, guest_entry_point
    mov rcx, VMCS_GUEST_RIP
    vmwrite rcx, rbx

    mov rbx, guest_stack_top
    mov rcx, VMCS_GUEST_RSP
    vmwrite rcx, rbx

    ; Guest RFLAGS (IF + other bits)
    mov rbx, 0x2  ; Interrupt enable flag
    mov rcx, VMCS_GUEST_RFLAGS
    vmwrite rcx, rbx

    ; VMCS link pointer (invalid = not nested)
    mov rbx, VMCS_LINK_PTR_INVALID
    mov rcx, VMCS_LINK_POINTER
    vmwrite rcx, rbx

    mov eax, 1
    jmp .done

.done:
    pop rcx
    pop rbx
    ret

; =============================================================================
; configure_vmcs_host - Setup host state fields
; Returns: EAX = 1 on success, 0 on failure
; =============================================================================
configure_vmcs_host:
    push rbx
    push rcx

    ; Host segment selectors
    mov rbx, 0x08  ; CS
    mov rcx, VMCS_HOST_CS_SELECTOR
    vmwrite rcx, rbx

    mov rbx, 0x10  ; DS, ES, SS
    mov rcx, VMCS_HOST_DS_SELECTOR
    vmwrite rcx, rbx
    mov rcx, VMCS_HOST_ES_SELECTOR
    vmwrite rcx, rbx
    mov rcx, VMCS_HOST_SS_SELECTOR
    vmwrite rcx, rbx

    mov rbx, 0
    mov rcx, VMCS_HOST_FS_SELECTOR
    vmwrite rcx, rbx
    mov rcx, VMCS_HOST_GS_SELECTOR
    vmwrite rcx, rbx

    ; Host control registers
    mov rbx, cr0
    mov rcx, VMCS_HOST_CR0
    vmwrite rcx, rbx

    mov rbx, cr3
    mov rcx, VMCS_HOST_CR3
    vmwrite rcx, rbx

    mov rbx, cr4
    mov rcx, VMCS_HOST_CR4
    vmwrite rcx, rbx

    ; Host RIP (where to return on VM exit)
    mov rbx, vmexit_handler
    mov rcx, VMCS_HOST_RIP
    vmwrite rcx, rbx

    ; Host RSP (stack for VM exit handler)
    mov rbx, host_stack_for_exit
    mov rcx, VMCS_HOST_RSP
    vmwrite rcx, rbx

    mov eax, 1
    jmp .done

.done:
    pop rcx
    pop rbx
    ret

; =============================================================================
; vmcs_write - Write a field to current VMCS
; RDI = VMCS field encoding (64-bit)
; RSI = Value to write
; Returns: EAX = 1 on success, 0 on failure
; =============================================================================
vmcs_write:
    vmwrite rdi, rsi

    ; Check for success (ZF=0)
    jnz .write_success

    xor eax, eax
    ret

.write_success:
    mov eax, 1
    ret

; =============================================================================
; vmcs_read - Read a field from current VMCS
; RDI = VMCS field encoding (64-bit)
; Returns: RAX = value read from VMCS
; =============================================================================
vmcs_read:
    vmread rdi, rax
    ret

; =============================================================================
; vmcs_clear - Wrapper for VMCLEAR
; RDI = pointer to VMCS region
; =============================================================================
vmcs_clear:
    push rbp
    mov rbp, rsp

    call do_vmclear

    pop rbp
    ret

; =============================================================================
; vmcs_load - Wrapper for VMPTRLD
; RDI = pointer to VMCS region
; =============================================================================
vmcs_load:
    push rbp
    mov rbp, rsp

    call do_vmptrld

    pop rbp
    ret

; =============================================================================
; vmcs_launch_state - Check VMCS launch state
; Returns: EAX = 0 if clear, 1 if launched
; =============================================================================
vmcs_launch_state:
    mov eax, [vmcs_launch_state_value]
    ret

; =============================================================================
; Guest Entry Point
; =============================================================================
section .text
global guest_entry_point
guest_entry_point:
    ; This is where the guest (host OS) will resume
    ; For this PoC, just halt
    hlt
    jmp guest_entry_point

; =============================================================================
; Data Section
; =============================================================================
section .bss

align 4096

; VMCS region (4KB aligned)
vmcs_region:
    resb 4096

; Pointer to VMCS region
vmcs_region_ptr:
    resq 1

; VMCS launch state (0=clear, 1=launched)
vmcs_launch_state_value:
    resd 1

; Host stack for VM exit
host_stack_for_exit:
    resq 1

; Guest stack
alignb 16
guest_stack_bottom:
    resb 16384
guest_stack_top:

; Additional control flags
section .data

; Control field definitions
control_fields:
    .pin_based:
        dq 0x0000  ; External interrupt exit + NMI exit
    .proc_based:
        dq 0x40000000  ; Secondary controls enabled + HLT exit
    .proc_based_secondary:
        dq 0x00000000  ; Secondary controls
    .vmexit:
        dq 0x00020000  ; Host address space size
    .vmentry:
        dq 0x00000200  ; IA-32e mode guest
