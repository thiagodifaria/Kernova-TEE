; =============================================================================
; Kernova-TEE - SIMD Marshalling Engine
; =============================================================================
; Ultra-fast binary marshalling between Host OS and Secure Enclave
; Uses AVX2/AVX-512 for parallel data processing
; =============================================================================

section .text
bits 64

; Include AVX macros
%include "avx_macros.inc"

; External symbols
extern get_vmx_region_ptr
extern data_enclave_encrypt

; Global symbols
global simd_pack_data
global simd_unpack_data
global simd_secure_memcpy
global simd_zero_memory
global simd_hash_blocks

; =============================================================================
; simd_pack_data - Pack data into SIMD registers for transfer
; RDI = destination buffer (enclave side)
; RSI = source buffer (host side)
; RDX = size in bytes
; RCX = flags (bit 0 = use AVX-512 if available)
; Returns: EAX = bytes transferred, -1 on error
; =============================================================================
simd_pack_data:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; Save parameters
    mov r12, rdi    ; dest
    mov r13, rsi    ; src
    mov r14, rdx    ; size
    mov r15, rcx    ; flags

    ; Validate inputs
    test r12, r12
    jz .invalid_param
    test r13, r13
    jz .invalid_param
    test r14, r14
    jz .zero_size
    jl .invalid_param

    ; Check if we should use AVX-512
    test r15, 1
    jnz .try_avx512

.use_avx2:
    ; Use AVX2 (256-bit) packing
    call simd_pack_256bit
    jmp .pack_done

.try_avx512:
    ; Check for AVX-512 support
    call check_avx512_support
    test eax, eax
    jz .use_avx2

    ; Use AVX-512 (512-bit) packing
    call simd_pack_512bit
    jmp .pack_done

.invalid_param:
    mov eax, -1
    jmp .pack_exit

.zero_size:
    xor eax, eax
    jmp .pack_exit

.pack_done:
    ; Clean up SIMD state
    vzeroupper

.pack_exit:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

; =============================================================================
; simd_pack_256bit - Pack data using AVX2 (256-bit at a time)
; R12 = dest, R13 = src, R14 = size
; Returns: EAX = bytes transferred
; =============================================================================
simd_pack_256bit:
    push rbp
    mov rbp, rsp

    ; Clean SIMD state
    vzeroupper

    xor rbx, rbx                ; byte counter
    xor rcx, rcx                ; loop counter

.pack_loop:
    ; Check if we have at least 32 bytes
    mov rax, r14
    sub rax, rbx
    cmp rax, 32
    jl .pack_remaining

    ; Load 32 bytes from source
    vmovdqu ymm0, [r13 + rbx]

    ; Optional: Apply transformation (encryption prep)
    ; For now, just store directly
    vmovdqu [r12 + rbx], ymm0

    add rbx, 32
    jmp .pack_loop

.pack_remaining:
    ; Handle remaining bytes (less than 32)
    mov rax, r14
    sub rax, rbx
    test rax, rax
    jz .pack_complete

    ; Use vector for remaining bytes
    cmp rax, 16
    jge .use_xmm

.use_byte_copy:
    ; Copy remaining bytes one at a time
    mov rcx, rax
    mov rsi, r13
    add rsi, rbx
    mov rdi, r12
    add rdi, rbx
    rep movsb
    jmp .pack_complete

.use_xmm:
    ; Use XMM for 16-31 bytes
    vmovdqu xmm0, [r13 + rbx]
    vmovdqu [r12 + rbx], xmm0
    add rbx, 16

    ; Check if more bytes remain
    mov rax, r14
    sub rax, rbx
    test rax, rax
    jz .pack_complete
    jmp .use_byte_copy

.pack_complete:
    ; Return bytes transferred
    mov rax, r14

    ; Clean up
    vzeroupper
    pop rbp
    ret

; =============================================================================
; simd_pack_512bit - Pack data using AVX-512 (512-bit at a time)
; R12 = dest, R13 = src, R14 = size
; Returns: EAX = bytes transferred
; =============================================================================
simd_pack_512bit:
    push rbp
    mov rbp, rsp

    xor rbx, rbx                ; byte counter

.pack_loop:
    ; Check if we have at least 64 bytes
    mov rax, r14
    sub rax, rbx
    cmp rax, 64
    jl .pack_remaining

    ; Load 64 bytes from source
    vmovdqu64 zmm0, [r13 + rbx]

    ; Store to destination
    vmovdqu64 [r12 + rbx], zmm0

    add rbx, 64
    jmp .pack_loop

.pack_remaining:
    ; Adjust source/dest/size for bytes already copied by AVX-512
    add r13, rbx            ; src += bytes_copied
    add r12, rbx            ; dest += bytes_copied
    sub r14, rbx            ; size -= bytes_copied
    test r14, r14
    jz .pack_512_done
    ; Fall back to AVX2 for remaining < 64 bytes
    jmp simd_pack_256bit

.pack_512_done:
    mov rax, r14
    pop rbp
    ret

; =============================================================================
; simd_unpack_data - Unpack data from SIMD registers
; RDI = destination buffer (host side)
; RSI = source buffer (enclave side)
; RDX = size in bytes
; RCX = flags
; Returns: EAX = bytes transferred, -1 on error
; =============================================================================
simd_unpack_data:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14

    ; Save parameters
    mov r12, rdi    ; dest
    mov r13, rsi    ; src
    mov r14, rdx    ; size

    ; Validate inputs
    test r12, r12
    jz .invalid_param
    test r13, r13
    jz .invalid_param
    test r14, r14
    jz .zero_size
    jl .invalid_param

    ; Clean SIMD state
    vzeroupper

    xor rbx, rbx                ; byte counter

.unpack_loop:
    ; Check if we have at least 32 bytes
    mov rax, r14
    sub rax, rbx
    cmp rax, 32
    jl .unpack_remaining

    ; Load 32 bytes from source
    vmovdqu ymm0, [r13 + rbx]

    ; Store to destination
    vmovdqu [r12 + rbx], ymm0

    add rbx, 32
    jmp .unpack_loop

.unpack_remaining:
    ; Handle remaining bytes
    mov rax, r14
    sub rax, rbx
    test rax, rax
    jz .unpack_complete

    mov rcx, rax
    mov rsi, r13
    add rsi, rbx
    mov rdi, r12
    add rdi, rbx
    rep movsb

.unpack_complete:
    mov rax, r14
    jmp .unpack_done

.invalid_param:
    mov eax, -1

.zero_size:
    xor eax, eax

.unpack_done:
    vzeroupper
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

; =============================================================================
; simd_secure_memcpy - Aligned memory copy with cache optimization
; RDI = destination
; RSI = source
; RDX = size
; Returns: EAX = bytes copied
; =============================================================================
simd_secure_memcpy:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14

    mov r12, rdi    ; dest
    mov r13, rsi    ; src
    mov r14, rdx    ; size

    ; Validate alignment for optimal performance
    test r12, 0x1F              ; Check 32-byte alignment
    jnz .unaligned_copy
    test r13, 0x1F
    jnz .unaligned_copy

.aligned_copy:
    ; Use aligned copy with prefetch
    xor rbx, rbx

.memcpy_loop:
    mov rax, r14
    sub rax, rbx
    cmp rax, 32
    jl .memcpy_remaining

    ; Prefetch next cache line
    prefetcht0 [r13 + rbx + 256]

    ; Copy 32 bytes
    vmovdqa ymm0, [r13 + rbx]
    vmovdqa [r12 + rbx], ymm0

    add rbx, 32
    jmp .memcpy_loop

.unaligned_copy:
    ; Fall through to same loop but with vmovdqu
    xor rbx, rbx

.unaligned_loop:
    mov rax, r14
    sub rax, rbx
    cmp rax, 32
    jl .memcpy_remaining

    vmovdqu ymm0, [r13 + rbx]
    vmovdqu [r12 + rbx], ymm0

    add rbx, 32
    jmp .unaligned_loop

.memcpy_remaining:
    ; Handle remaining bytes
    mov rax, r14
    sub rax, rbx
    test rax, rax
    jz .memcpy_done

    mov rcx, rax
    lea rsi, [r13 + rbx]
    lea rdi, [r12 + rbx]
    rep movsb

.memcpy_done:
    mov rax, r14
    vzeroupper
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

; =============================================================================
; simd_zero_memory - Securely zero memory using SIMD
; RDI = buffer to zero
; RSI = size in bytes
; Returns: EAX = bytes zeroed
; =============================================================================
simd_zero_memory:
    push rbp
    mov rbp, rsp
    push rbx
    push rdi
    push rsi

    ; Validate inputs
    test rdi, rdi
    jz .zero_done
    test rsi, rsi
    jz .zero_done

    ; Create zero vector
    vpxor ymm0, ymm0, ymm0

    xor rbx, rbx                ; byte counter

.zero_loop:
    mov rax, rsi
    sub rax, rbx
    cmp rax, 32
    jl .zero_remaining

    ; Zero 32 bytes at a time
    vmovdqu [rdi + rbx], ymm0

    add rbx, 32
    jmp .zero_loop

.zero_remaining:
    ; Zero remaining bytes
    mov rax, rsi
    sub rax, rbx
    test rax, rax
    jz .zero_complete

    ; Use rep stosb for remaining
    add rdi, rbx              ; rdi = buffer + offset
    mov rcx, rax
    xor eax, eax
    rep stosb

.zero_complete:
    mov rax, rsi

.zero_done:
    vzeroupper
    pop rsi
    pop rdi
    pop rbx
    pop rbp
    ret

; =============================================================================
; simd_hash_blocks - Calculate hash for data blocks using SIMD
; RDI = data buffer
; RSI = size
; RDX = hash output buffer (32 bytes)
; Returns: EAX = 0 on success, -1 on error
; =============================================================================
simd_hash_blocks:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r12, rdi    ; data
    mov r13, rsi    ; size
    mov r14, rdx    ; hash output

    ; Non-cryptographic 256-bit integrity hash for development tests.
    ; This is intentionally stronger than a plain XOR accumulator so small
    ; input changes spread across all output lanes.
    test r14, r14
    jz .invalid_param
    test r12, r12
    jz .invalid_data

    mov r8,  0x243f6a8885a308d3
    mov r9,  0x13198a2e03707344
    mov r10, 0xa4093822299f31d0
    mov r11, 0x082efa98ec4e6c89
    xor r8, r13
    mov rax, r13
    shl rax, 17
    xor r9, rax
    mov r15, 0x100000001b3
    xor rbx, rbx

.hash_loop:
    cmp rbx, r13
    jae .hash_finalize

    movzx rax, byte [r12 + rbx]

    mov rcx, rax
    add rcx, rbx
    xor r8, rcx
    imul r8, r15
    rol r8, 13

    mov rcx, rbx
    shl rcx, 8
    xor rcx, rax
    add r9, rcx
    imul r9, r15
    rol r9, 29

    mov rcx, rax
    shl rcx, 32
    xor rcx, rbx
    xor r10, rcx
    imul r10, r15
    rol r10, 37

    lea rcx, [rax + rbx]
    xor rcx, r11
    imul rcx, r15
    xor r11, rcx
    rol r11, 43

    inc rbx
    jmp .hash_loop

.hash_finalize:
    ; MurmurHash3-style finalizers per 64-bit lane.
    mov rax, r8
    call .fmix64
    mov [r14], rax

    mov rax, r9
    call .fmix64
    mov [r14 + 8], rax

    mov rax, r10
    call .fmix64
    mov [r14 + 16], rax

    mov rax, r11
    call .fmix64
    mov [r14 + 24], rax

    xor eax, eax
    jmp .hash_done

.invalid_data:
    test r13, r13
    jz .hash_zero

.invalid_param:
    mov eax, -1
    jmp .hash_done

.hash_zero:
    mov rax, 0x243f6a8885a308d3
    mov [r14], rax
    mov rax, 0x13198a2e03707344
    mov [r14 + 8], rax
    mov rax, 0xa4093822299f31d0
    mov [r14 + 16], rax
    mov rax, 0x082efa98ec4e6c89
    mov [r14 + 24], rax
    xor eax, eax
    jmp .hash_done

.fmix64:
    mov rcx, rax
    shr rcx, 33
    xor rax, rcx
    mov rcx, 0xff51afd7ed558ccd
    imul rax, rcx
    mov rcx, rax
    shr rcx, 33
    xor rax, rcx
    mov rcx, 0xc4ceb9fe1a85ec53
    imul rax, rcx
    mov rcx, rax
    shr rcx, 33
    xor rax, rcx
    ret

.hash_done:
    vzeroupper
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

; =============================================================================
; check_avx512_support - Check if CPU supports AVX-512
; Returns: EAX = 1 if supported, 0 otherwise
; =============================================================================
check_avx512_support:
    push rbx
    push rcx

    ; Check CPUID for AVX-512F
    mov eax, 7
    mov ecx, 0
    cpuid

    test ebx, (1 << 16)  ; Check AVX-512F bit
    jnz .supported

    xor eax, eax
    jmp .done

.supported:
    mov eax, 1

.done:
    pop rcx
    pop rbx
    ret

; =============================================================================
; Data Section
; =============================================================================
section .bss

align 64

; Temporary buffers for SIMD operations
simd_temp_buffer:
    resb 512

; Statistics (optional)
simd_stats:
    .pack_operations:
        resq 1
    .unpack_operations:
        resq 1
    .bytes_transferred:
        resq 1
