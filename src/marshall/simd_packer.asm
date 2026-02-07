; =============================================================================
; Micro-Hypervisor - SIMD Marshalling Engine
; =============================================================================
; Ultra-fast binary marshalling between Host OS and Secure Enclave
; Uses AVX2/AVX-512 for parallel data processing
; =============================================================================

section .text
bits 64

; Include AVX macros
%include "../../include/avx_macros.inc"

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

    ; Initialize pack context
    SIMD_PACK_INIT

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
    mov eax, r14

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
    ; Fall back to AVX2 for remaining
    jmp simd_pack_256bit

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

    ; Initialize unpack context
    SIMD_PACK_INIT

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
    mov eax, r14
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
    rep movsb

.memcpy_done:
    mov eax, r14
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
    mov rcx, rax
    xor eax, eax
    rep stosb

.zero_complete:
    mov eax, rsi

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

    mov r12, rdi    ; data
    mov r13, rsi    ; size
    mov r14, rdx    ; hash output

    ; Simple hash for demonstration (use proper crypto in production!)
    ; This is a basic XOR hash for demonstration purposes

    ; Initialize hash values
    vpxor ymm0, ymm0, ymm0    ; h0, h1, h2, h3 = 0
    vpxor ymm1, ymm1, ymm1    ; h4, h5, h6, h7 = 0

    xor rbx, rbx

.hash_loop:
    mov rax, r13
    sub rax, rbx
    cmp rax, 32
    jl .hash_remaining

    ; Load 32 bytes and XOR into hash
    vmovdqu ymm2, [r12 + rbx]
    vpxor ymm0, ymm0, ymm2

    add rbx, 32
    jmp .hash_loop

.hash_remaining:
    ; Handle remaining bytes
    mov rax, r13
    sub rax, rbx
    test rax, rax
    jz .hash_output

    ; XOR remaining bytes into hash
    mov rcx, rax
    xor eax, eax
.hash_byte_loop:
    xor al, [r12 + rbx]
    ; Insert into hash (simplified)
    inc rbx
    dec rcx
    jg .hash_byte_loop

.hash_output:
    ; Store hash result
    vmovdqu [r14], ymm0

    xor eax, eax

.hash_done:
    vzeroupper
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
