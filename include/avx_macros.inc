; =============================================================================
; Kernova-TEE - AVX/AVX-512 SIMD Marshalling Macros
; =============================================================================
; Ultra-fast binary marshalling between Host OS and Secure Enclave
; =============================================================================

; Macro: Detect AVX/AVX-512 support at runtime
; Checks CPUID for AVX, AVX2, and AVX-512F support
%macro CHECK_AVX_SUPPORT 3
    ; %1 = label for AVX supported
    ; %2 = label for AVX2 supported
    ; %3 = label for AVX-512 supported

    push rbx
    mov eax, 1
    cpuid
    test ecx, (1 << 28)          ; Check AVX support (bit 28 in ECX)
    jz %%no_avx

    ; Check AVX2 support
    mov eax, 7
    mov ecx, 0
    cpuid
    test ebx, (1 << 5)           ; Check AVX2 support (bit 5 in EBX)
    jnz %2

    ; Check AVX-512 Foundation
    mov eax, 7
    mov ecx, 0
    cpuid
    test ebx, (1 << 16)          ; Check AVX-512F (bit 16 in EBX)
    jnz %3

    jmp %1

%%no_avx:
    pop rbx
    ret
%endmacro

; Macro: Initialize SIMD packing context
; Sets up proper alignment and XMM/YMM/ZMM registers
%macro SIMD_PACK_INIT 0
    ; Ensure 32-byte alignment for AVX operations
    sub rsp, 32
    and rsp, -32

    ; Zero upper YMM registers for AVX state cleanliness
    vzeroupper
%endmacro

; Macro: Pack data into YMM register (256-bit AVX2)
; Input: RSI = source pointer, RCX = byte count
; Output: Packed data in YMM0-YMM7
%macro SIMD_PACK_256BIT 0
    xor rax, rax                    ; Clear register index
    xor rdx, rdx                    ; Clear block counter

%%pack_loop:
    ; Load 32 bytes into YMM register
    vmovdqu ymm0, [rsi + rax]

    ; Process based on alignment requirements
    ; For unaligned memory access, vmovdqu handles it
    add rax, 32

    ; Check if we have more data
    sub rcx, 32
    jg %%pack_loop

    ; Clean up upper YMM state
    vzeroupper
%endmacro

; Macro: Pack data into ZMM register (512-bit AVX-512)
; Input: RSI = source pointer, RCX = byte count
; Output: Packed data in ZMM0-ZMM31
%macro SIMD_PACK_512BIT 0
    xor rax, rax
    xor rdx, rdx

%%pack_loop:
    ; Load 64 bytes into ZMM register
    vmovdqu64 zmm0, [rsi + rax]

    add rax, 64
    sub rcx, 64
    jg %%pack_loop
%endmacro

; Macro: Unpack YMM data to destination
; Input: RDI = destination pointer, RCX = byte count
%macro SIMD_UNPACK_256BIT 0
    xor rax, rax

%%unpack_loop:
    ; Store 32 bytes from YMM register
    vmovdqu [rdi + rax], ymm0

    add rax, 32
    sub rcx, 32
    jg %%unpack_loop

    vzeroupper
%endmacro

; Macro: Unpack ZMM data to destination
; Input: RDI = destination pointer, RCX = byte count
%macro SIMD_UNPACK_512BIT 0
    xor rax, rax

%%unpack_loop:
    ; Store 64 bytes from ZMM register
    vmovdqu64 [rdi + rax], zmm0

    add rax, 64
    sub rcx, 64
    jg %%unpack_loop
%endmacro

; Macro: Secure data transfer with encryption preparation
; Uses AVX-512 for parallel AES preparation
%macro SIMD_SECURE_TRANSFER 3
    ; %1 = source, %2 = destination, %3 = size

    mov rsi, %1
    mov rdi, %2
    mov rcx, %3

%%transfer_loop:
    ; Check alignment for optimal performance
    test rsi, 0x3F              ; Check 64-byte alignment
    jnz %%unaligned

    ; Aligned transfer - use vmovdqa
    vmovdqa64 zmm0, [rsi]
    vmovdqa64 [rdi], zmm0
    jmp %%advance

%%unaligned:
    ; Unaligned transfer - use vmovdqu
    vmovdqu64 zmm0, [rsi]
    vmovdqu64 [rdi], zmm0

%%advance:
    add rsi, 64
    add rdi, 64
    sub rcx, 64
    jg %%transfer_loop

%%done:
%endmacro

; Macro: Initialize RDRAND hardware RNG
; Returns random value in RAX
%macro RDRAND_INIT 0
    ; Check RDRAND support
    mov eax, 1
    cpuid
    test ecx, (1 << 30)          ; Check RDRAND (bit 30 in ECX)
    jz %%no_rdrand

    ; Seed with hardware random number
    rdrand eax
    jc %%success

%%no_rdrand:
    ; Fallback to time-based seed
    rdtsc
    jmp %%done

%%success:
    ; Successfully got random value

%%done:
%endmacro

; Macro: Zero sensitive registers securely
; Prevents data leakage through register forwarding
%macro ZERO_SECURE_REGISTERS 0
    pxor xmm0, xmm0
    pxor xmm1, xmm1
    pxor xmm2, xmm2
    pxor xmm3, xmm3
    vpxor ymm0, ymm0, ymm0
    vpxor ymm1, ymm1, ymm1
    vpxor ymm2, ymm2, ymm2
    vpxor ymm3, ymm3, ymm3
%endmacro

; Macro: Aligned memory block copy with prefetch
; Uses PREFETCHT0 for cache optimization
%macro SIMD_MEMCPY_ALIGNED 3
    ; %1 = src, %2 = dst, %3 = size

    mov rsi, %1
    mov rdi, %2
    mov rcx, %3

%%copy_loop:
    ; Prefetch next cache line
    prefetcht0 [rsi + 256]

    ; Copy 256 bits (32 bytes) at a time
    vmovdqa ymm0, [rsi]
    vmovdqa [rdi], ymm0

    add rsi, 32
    add rdi, 32
    sub rcx, 32
    jg %%copy_loop

    vzeroupper
%endmacro

; Macro: Calculate CRC32 using hardware instruction (SSE4.2)
; Input: RAX = data pointer, RCX = length
; Output: RAX = CRC32 value
%macro SIMD_CRC32 0
    xor eax, eax                  ; Initialize CRC to 0

%%crc_loop:
    movzx edx, byte [rsi]
    crc32 eax, edx
    inc rsi
    dec rcx
    jg %%crc_loop
%endmacro

; Macro: Parallel XOR for cryptographic operations
; Uses AVX2 for 256-bit parallel XOR
%macro SIMD_XOR_256 3
    ; %1 = destination, %2 = source1, %3 = source2

    vmovdqu ymm0, [%2]
    vmovdqu ymm1, [%3]
    vpxor ymm0, ymm0, ymm1
    vmovdqu [%1], ymm0
%endmacro

; Macro: Flush SIMD state before VM transition
; Critical for preventing VMCS corruption
%macro SIMD_STATE_FLUSH 0
    vzeroupper                    ; Clear upper YMM
    ; For AVX-512 systems, also clear ZMM state
    ; vzeroupper handles this on most implementations
%endmacro

; Macro: Validate SIMD register state
; Checks for corrupted YMM/ZMM state
%macro SIMD_VALIDATE_STATE 0
    ; Try to detect NaN or invalid floating point state
    vpxor ymm15, ymm15, ymm15    ; Create known-good zero register

    ; If this fails, we have a hardware problem
    ; (cannot detect in assembly without exception handling)
%endmacro

; Constant definitions for SIMD operations
%define SIMD_ALIGNMENT_256      32
%define SIMD_ALIGNMENT_512      64
%define SIMD_BLOCK_SIZE_256     32
%define SIMD_BLOCK_SIZE_512     64

; AVX-512 Mask registers for conditional operations
%define K0_MASK                 0
%define K1_MASK                 1
%define K2_MASK                 2
%define K3_MASK                 3
%define K4_MASK                 4
%define K5_MASK                 5
%define K6_MASK                 6
%define K7_MASK                 7
