; =============================================================================
; Kernova-TEE - VMCS Field Definitions
; =============================================================================
; Based on Intel SDM Volume 3C - Chapter 24 VMCS Fields
; =============================================================================

; VMCS Field Encoding - 16-bit encoding
; Bits [15:14] - Type (00=Control, 01=VM-Exit Info, 10=VM-Entry, 11=Guest State)
; Bits [13:11] - Type-specific
; Bits [10:0]  - Index

; VMCS Control Fields (Type = 00)
%define VMCS_CTRL_16BIT           0x0000
%define VMCS_CTRL_32BIT           0x4000
%define VMCS_CTRL_64BIT           0x2000
%define VMCS_CTRL_NATURAL_WIDTH   0x6000
%define VM_CONTROL_16BIT          0x0000
%define VM_CONTROL_64BIT          0x4000
%define VM_CONTROL_32BIT          0x6000
%define VM_CONTROL_NATURAL_WIDTH  0x8000

; Primary Processor-Based VM-Execution Controls
%define VCPU_PIN_BASED            0x4000
%define VCPU_PROC_BASED           0x4002
%define VCPU_EXCEPTION_BITMAP     0x4004
%define VCPU_VMEXIT_CONTROLS      0x400C
%define VCPU_VMENTRY_CONTROLS     0x4012
%define VCPU_CR3_TARGET_COUNT     0x4000
%define VCPU_CR3_TARGET0          0x6000

; VMCS Field Identifiers - 64-bit Control Fields
%define VMCS_CTRL_IO_BITMAP_A     0x2000
%define VMCS_CTRL_IO_BITMAP_B     0x2002
%define VMCS_CTRL_MSR_BITMAP      0x2004
%define VMCS_CTRL_EPT_POINTER     0x201A

; VMCS Field Identifiers - 16-bit Control Fields
%define VMCS_CTRL_VPID            0x0000

; VMCS Field Identifiers - Guest State
%define VMCS_GUEST_ES_SELECTOR    0x0800
%define VMCS_GUEST_CS_SELECTOR    0x0802
%define VMCS_GUEST_SS_SELECTOR    0x0804
%define VMCS_GUEST_DS_SELECTOR    0x0806
%define VMCS_GUEST_FS_SELECTOR    0x0808
%define VMCS_GUEST_GS_SELECTOR    0x080A
%define VMCS_GUEST_LDTR_SELECTOR  0x080C
%define VMCS_GUEST_TR_SELECTOR    0x080E

%define VMCS_GUEST_ES_LIMIT       0x4800
%define VMCS_GUEST_CS_LIMIT       0x4802
%define VMCS_GUEST_SS_LIMIT       0x4804
%define VMCS_GUEST_DS_LIMIT       0x4806
%define VMCS_GUEST_FS_LIMIT       0x4808
%define VMCS_GUEST_GS_LIMIT       0x480A
%define VMCS_GUEST_LDTR_LIMIT     0x480C
%define VMCS_GUEST_TR_LIMIT       0x480E

%define VMCS_GUEST_CR0            0x6800
%define VMCS_GUEST_CR3            0x6802
%define VMCS_GUEST_CR4            0x6804
%define VMCS_GUEST_RSP            0x681C
%define VMCS_GUEST_RIP            0x681E
%define VMCS_GUEST_RFLAGS         0x6820

; VMCS Field Identifiers - Host State
%define VMCS_HOST_ES_SELECTOR     0x0C00
%define VMCS_HOST_CS_SELECTOR     0x0C02
%define VMCS_HOST_SS_SELECTOR     0x0C04
%define VMCS_HOST_DS_SELECTOR     0x0C06
%define VMCS_HOST_FS_SELECTOR     0x0C08
%define VMCS_HOST_GS_SELECTOR     0x0C0A
%define VMCS_HOST_TR_SELECTOR     0x0C0C

%define VMCS_HOST_CR0             0x6C00
%define VMCS_HOST_CR3             0x6C02
%define VMCS_HOST_CR4             0x6C04
%define VMCS_HOST_RSP             0x6C1C
%define VMCS_HOST_RIP             0x6C1E

; VMCS Guest Register State (for CPUID intercept)
%define VMCS_GUEST_RAX            0x681A
%define VMCS_GUEST_RBX            0x6818
%define VMCS_GUEST_RCX            0x681C
%define VMCS_GUEST_RDX            0x681E

; VMCS Link Pointer
%define VMCS_LINK_POINTER         0x2800

; VMCS Field Identifiers - VM-Exit Information
%define VMCS_EXIT_REASON          0x4402
%define VMCS_EXIT_QUALIFICATION   0x6400
%define VMCS_GUEST_LINEAR_ADDRESS 0x6402
%define VMCS_EXIT_INSTR_LEN       0x440C

; VMCS Field Identifiers - VM-Entry Controls
%define VMCS_ENTRY_CONTROLS       0x4012

; CR0 Bits
%define CR0_PE                    0x0001   ; Protection Enable
%define CR0_WP                    0x10000  ; Write Protect

; CR4 Bits
%define CR4_VMXE                  0x2000   ; Enable VMX operation

; VMCS Link Pointer Values
%define VMCS_LINK_PTR_INVALID     0xFFFFFFFFFFFFFFFF

; VM-Exit Reasons
%define EXIT_REASON_EXCEPTION_NMI 0
%define EXIT_REASON_EXTERNAL_INTERRUPT 1
%define EXIT_REASON_TRIPLE_FAULT  2
%define EXIT_REASON_INIT_SIGNAL   3
%define EXIT_REASON_SIPI          4
%define EXIT_REASON_IO_SMI        5
%define EXIT_REASON_OTHER_SMI     6
%define EXIT_REASON_INTERRUPT_WINDOW 7
%define EXIT_REASON_NMI_WINDOW    8
%define EXIT_REASON_TASK_SWITCH   9
%define EXIT_REASON_CPUID         10
%define EXIT_REASON_GETSEC        11
%define EXIT_REASON_HLT           12
%define EXIT_REASON_INVD          13
%define EXIT_REASON_INVLPG        14
%define EXIT_REASON_RDPMC         15
%define EXIT_REASON_RDTSC         16
%define EXIT_REASON_RSM           17
%define EXIT_REASON_VMCALL        18
%define EXIT_REASON_VMCLEAR       19
%define EXIT_REASON_VMLAUNCH      20
%define EXIT_REASON_VMPTRLD       21
%define EXIT_REASON_VMPTRST       22
%define EXIT_REASON_VMREAD        23
%define EXIT_REASON_VMRESUME      24
%define EXIT_REASON_VMWRITE       25
%define EXIT_REASON_VMXOFF        26
%define EXIT_REASON_VMXON         27
%define EXIT_REASON_CR_ACCESS     28
%define EXIT_REASON_DR_ACCESS     29
%define EXIT_REASON_IO_INSTR      30
%define EXIT_REASON_RDMSR         31
%define EXIT_REASON_WRMSR         32
%define EXIT_REASON_MSR_WRITE      32      ; Alias for WRMSR
%define EXIT_REASON_ENTRY_FAIL_GUEST 33
%define EXIT_REASON_ENTRY_FAIL_MSR  34
%define EXIT_REASON_MWAIT         36
%define EXIT_REASON_MTF           37
%define EXIT_REASON_MONITOR       39
%define EXIT_REASON_PAUSE         40
%define EXIT_REASON_ENTRY_FAIL_MC  41

; Processor-Based VM-Execution Controls
%define PROC_BASED_CR3_LOAD       (1 << 0)
%define PROC_BASED_CR3_STORE      (1 << 1)
%define PROC_BASED_HLT_EXIT       (1 << 7)
%define PROC_BASED_SECONDARY_CTLS  (1 << 31)

; Pin-Based Controls
%define PIN_BASED_EXT_INTR_EXIT   (1 << 0)
%define PIN_BASED_NMI_EXIT        (1 << 3)

; VM-Entry Controls
%define ENTRY_CONTROLS_IA64_MODE  (1 << 9)

; VM-Exit Controls
%define EXIT_CONTROLS_HOST_ADDR64 (1 << 9)

; Secondary Processor-Based VM-Execution Controls
%define PROC_BASED2_ENABLE_EPT    (1 << 1)
%define PROC_BASED2_ENABLE_VPID   (1 << 5)

; VMX MSR Addresses
%define IA32_VMX_BASIC            0x480
%define IA32_VMX_PINBASED_CTLS    0x481
%define IA32_VMX_PROCBASED_CTLS   0x482
%define IA32_VMX_EXIT_CTLS        0x483
%define IA32_VMX_ENTRY_CTLS       0x484
%define IA32_FEATURE_CONTROL      0x3A

; Struct definitions for C++ interface
struc VMXState
    .vmxon_region: resq 1    ; 4KB aligned region for VMXON
    .vmcs_region: resq 1     ; 4KB aligned region for VMCS
    .vmcs_launch_state: resd 1
    .revision_id: resd 1
    .reserved: resd 8
endstruc

; MSR Bitmaps Configuration
%define MSR_BITMAP_SIZE         0x1000   ; 4KB for MSR bitmap

; Segment Access Rights
%define SEG_AR_TYPE_MASK        0x0F
%define SEG_AR_S_MASK           0x10
%define SEG_AR_DPL_MASK         0x60
%define SEG_AR_P_MASK           0x80
%define SEG_AR_AVL_MASK         0x100
%define SEG_AR_L_MASK           0x200
%define SEG_AR_DB_MASK          0x400
%define SEG_AR_G_MASK           0x800

; Access Rights for Code Segment (Readable, Accessed)
%define CODE_SEG_AR             0xA09B   ; P=1, DPL=0, S=1, Type=1011
%define DATA_SEG_AR             0xC093   ; P=1, DPL=0, S=1, Type=0011
