; =============================================================================
; Micro-Hypervisor - Hardware Trace Engine & Integrity Monitor
; =============================================================================
; Monitors kernel integrity and detects rootkit persistence mechanisms
; Uses hardware breakpoints and page table monitoring
; =============================================================================

section .text
bits 64

; External symbols
extern simd_hash_blocks

; Global symbols
global trace_init
global trace_log_event
global trace_check_integrity
global trace_set_watchpoint
global trace_get_statistics

; =============================================================================
; Trace Event Types
; =============================================================================
section .rodata

; Event severity levels
%define TRACE_SEVERITY_INFO    0
%define TRACE_SEVERITY_WARNING 1
%define TRACE_SEVERITY_CRITICAL 2

; Event type IDs
%define TRACE_EVENT_INIT       0
%define TRACE_EVENT_CR_WRITE   1
%define TRACE_EVENT_MSR_WRITE  2
%define TRACE_EVENT_HOOK_DETECT 3
%define TRACE_EVENT_VMEXIT     4
%define TRACE_EVENT_INTEGRITY_FAIL 5

; =============================================================================
; trace_init - Initialize the trace engine
; Returns: EAX = 0 on success, -1 on failure
; =============================================================================
trace_init:
    push rbp
    mov rbp, rsp
    push rbx
    push rdi

    ; Initialize trace buffer
    mov rdi, trace_buffer
    mov rcx, TRACE_BUFFER_SIZE
    call simd_zero_memory

    ; Initialize statistics
    mov qword [trace_stats.event_count], 0
    mov qword [trace_stats.critical_events], 0
    mov qword [trace_stats.integrity_checks], 0
    mov qword [trace_stats.integrity_failures], 0

    ; Set up initial watchpoints on sensitive regions
    ; 1. IDT (Interrupt Descriptor Table)
    ; 2. GDT (Global Descriptor Table)
    ; 3. Syscall handler (MSR.LSTAR)

    ; Initialize baseline hashes
    call compute_baseline_hashes

    xor eax, eax
    jmp .done

.done:
    pop rdi
    pop rbx
    pop rbp
    ret

; =============================================================================
; trace_log_event - Log an event to the trace buffer
; RDI = event type
; RSI = event-specific data
; Returns: EAX = 0 on success, -1 on failure
; =============================================================================
trace_log_event:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; Save parameters
    mov r12, rdi    ; event type
    mov r13, rsi    ; event data

    ; Check if trace buffer is full
    mov rax, [trace_buffer.write_index]
    cmp rax, TRACE_MAX_EVENTS
    jge .buffer_full

    ; Get pointer to next event slot
    mov rbx, trace_buffer.events
    shl rax, 5                    ; Each event is 32 bytes
    lea rdi, [rbx + rax]          ; RDI = event slot

    ; Fill in event structure
    ; Timestamp
    rdtsc
    mov [rdi + TraceEvent.timestamp], rax

    ; Event type
    mov [rdi + TraceEvent.type], r12b

    ; Data
    mov [rdi + TraceEvent.data], r13

    ; Update write index
    lock inc qword [trace_buffer.write_index]

    ; Update statistics
    lock inc qword [trace_stats.event_count]

    ; Check if this is a critical event
    cmp r12, TRACE_SEVERITY_CRITICAL
    jne .not_critical

    lock inc qword [trace_stats.critical_events]

.not_critical:
    xor eax, eax
    jmp .done

.buffer_full:
    mov eax, -1

.done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

; =============================================================================
; trace_check_integrity - Check kernel integrity
; Returns: EAX = 0 if integrity OK, -1 if compromised
; =============================================================================
trace_check_integrity:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14

    ; Increment check counter
    lock inc qword [trace_stats.integrity_checks]

    ; Check 1: Verify IDT hasn't been modified
    mov rdi, "IDT "
    call check_region_integrity
    test eax, eax
    jz .idt_compromised

    ; Check 2: Verify GDT integrity
    mov rdi, "GDT "
    call check_region_integrity
    test eax, eax
    jz .gdt_compromised

    ; Check 3: Verify syscall handler (MSR.LSTAR target)
    mov rdi, "SYSC"
    call check_region_integrity
    test eax, eax
    jz .syscall_compromised

    ; All checks passed
    xor eax, eax
    jmp .done

.idt_compromised:
    mov rdi, TRACE_EVENT_INTEGRITY_FAIL
    mov rsi, 0x01                ; IDT failure code
    call trace_log_event

    lock inc qword [trace_stats.integrity_failures]
    mov eax, -1
    jmp .done

.gdt_compromised:
    mov rdi, TRACE_EVENT_INTEGRITY_FAIL
    mov rsi, 0x02                ; GDT failure code
    call trace_log_event

    lock inc qword [trace_stats.integrity_failures]
    mov eax, -1
    jmp .done

.syscall_compromised:
    mov rdi, TRACE_EVENT_INTEGRITY_FAIL
    mov rsi, 0x03                ; Syscall failure code
    call trace_log_event

    lock inc qword [trace_stats.integrity_failures]
    mov eax, -1

.done:
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

; =============================================================================
; check_region_integrity - Check if a memory region has been modified
; RDI = region identifier (4-byte tag)
; Returns: EAX = 1 if OK, 0 if modified
; =============================================================================
check_region_integrity:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14

    ; Save region tag
    mov r12, rdi

    ; Find baseline hash for this region
    mov rbx, integrity_baseline
    mov r14, 5                   ; Check up to 5 baseline entries

.find_loop:
    test r14, r14
    jz .not_found

    ; Compare tags
    mov rax, [rbx + IntegrityEntry.tag]
    cmp rax, r12
    je .found

    add rbx, IntegrityEntry_size
    dec r14
    jmp .find_loop

.not_found:
    ; No baseline found - can't check
    mov eax, 1                   ; Assume OK
    jmp .done

.found:
    ; Compute current hash
    mov rdi, [rbx + IntegrityEntry.address]
    mov rsi, [rbx + IntegrityEntry.size]
    mov rdx, current_hash_buffer
    call simd_hash_blocks

    ; Compare with baseline
    mov rdi, current_hash_buffer
    mov rsi, rbx
    add rsi, IntegrityEntry.hash
    mov rcx, 32                 ; 256-bit hash
    call compare_memory

    test eax, eax
    jz .modified

    ; Hash matches - integrity OK
    mov eax, 1
    jmp .done

.modified:
    ; Hash mismatch - region modified!
    xor eax, eax

.done:
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

; =============================================================================
; trace_set_watchpoint - Set a hardware watchpoint on an address
; RDI = address to watch
    ; RSI = size (1, 2, 4, or 8 bytes)
; RDX = type (0=write, 1=read/write)
; Returns: EAX = 0 on success, -1 on failure
; =============================================================================
trace_set_watchpoint:
    push rbp
    mov rbp, rsp
    push rbx
    push rdx

    ; Use DR0-DR3 for watchpoints
    ; For this PoC, we'll just note the address
    ; In production, would program debug registers

    ; Validate size
    mov rax, rsi
    cmp rax, 1
    je .size_ok
    cmp rax, 2
    je .size_ok
    cmp rax, 4
    je .size_ok
    cmp rax, 8
    je .size_ok

    ; Invalid size
    mov eax, -1
    jmp .done

.size_ok:
    ; Store watchpoint info
    mov rbx, watchpoints
    mov rcx, 4                  ; 4 watchpoints available

.find_free:
    test rcx, rcx
    jz .no_free

    cmp qword [rbx + Watchpoint.address], 0
    je .found_free

    add rbx, Watchpoint_size
    dec rcx
    jmp .find_free

.no_free:
    mov eax, -1
    jmp .done

.found_free:
    ; Set up watchpoint
    mov [rbx + Watchpoint.address], rdi
    mov [rbx + Watchpoint.size], sil
    mov [rbx + Watchpoint.type], dl

    ; In production, would program DR0-DR3 here
    ; mov dr0, rdi
    ; mov rax, dr7
    ; or rax, (1 << 0)  ; Enable DR0
    ; mov dr7, rax

    xor eax, eax

.done:
    pop rdx
    pop rbx
    pop rbp
    ret

; =============================================================================
; trace_get_statistics - Get trace engine statistics
; RDI = pointer to statistics structure to fill
; Returns: EAX = 0 on success
; =============================================================================
trace_get_statistics:
    push rbp
    mov rbp, rsp
    push rdi
    push rsi
    push rcx

    ; Copy statistics to user buffer
    mov rsi, trace_stats
    mov rcx, TraceStatistics_size
    rep movsb

    xor eax, eax

    pop rcx
    pop rsi
    pop rdi
    pop rbp
    ret

; =============================================================================
; compute_baseline_hashes - Compute initial hashes for critical regions
; =============================================================================
compute_baseline_hashes:
    push rbp
    mov rbp, rsp
    push rbx

    ; For this PoC, we'll set up fake baselines
    ; In production, would compute actual hashes of IDT, GDT, etc.

    mov rbx, integrity_baseline

    ; IDT baseline
    mov qword [rbx + IntegrityEntry.tag], "IDT "
    mov qword [rbx + IntegrityEntry.address], 0
    mov qword [rbx + IntegrityEntry.size], 256
    ; Zero hash for now
    mov rdi, rbx
    add rdi, IntegrityEntry.hash
    mov rcx, 32
    xor eax, eax
    rep stosb

    ; GDT baseline
    add rbx, IntegrityEntry_size
    mov qword [rbx + IntegrityEntry.tag], "GDT "
    mov qword [rbx + IntegrityEntry.address], 0
    mov qword [rbx + IntegrityEntry.size], 256

    ; Syscall handler baseline
    add rbx, IntegrityEntry_size
    mov qword [rbx + IntegrityEntry.tag], "SYSC"
    mov qword [rbx + IntegrityEntry.address], 0
    mov qword [rbx + IntegrityEntry.size], 256

    pop rbx
    pop rbp
    ret

; =============================================================================
; compare_memory - Constant-time memory compare
; RDI = buffer1
; RSI = buffer2
; RCX = length
; Returns: EAX = 1 if equal, 0 if different
; =============================================================================
compare_memory:
    push rbx
    xor rbx, rbx                ; result accumulator

.compare_loop:
    test rcx, rcx
    jz .done

    mov al, [rdi]
    mov bl, [rsi]
    cmp al, bl
    je .equal

    xor rbx, 1                  ; Mark as different

.equal:
    inc rdi
    inc rsi
    dec rcx
    jmp .compare_loop

.done:
    test rbx, rbx
    setz al                     ; Set if zero (equal)
    movzx eax, al

    pop rbx
    ret

; =============================================================================
; Data Structures
; =============================================================================

; Trace Event Structure
struc TraceEvent
    .timestamp:    resq 1       ; 8 bytes
    .type:         resb 1       ; 1 byte
    .severity:     resb 1       ; 1 byte
    .reserved:     resb 2       ; 2 bytes
    .data:         resq 1       ; 8 bytes
    ; Total: 20 bytes, rounded to 32 for alignment
endstruc

; Integrity Entry Structure
struc IntegrityEntry
    .tag:          resq 1       ; 8 bytes - region identifier
    .address:      resq 1       ; 8 bytes - region start
    .size:         resq 1       ; 8 bytes - region size
    .hash:         resb 32      ; 32 bytes - SHA-256 hash
endstruc
IntegrityEntry_size equ 56

; Watchpoint Structure
struc Watchpoint
    .address:      resq 1       ; 8 bytes
    .size:         resq 1       ; 8 bytes
    .type:         resq 1       ; 8 bytes
endstruc
Watchpoint_size equ 24

; Trace Statistics Structure
struc TraceStatistics
    .event_count:       resq 1
    .critical_events:   resq 1
    .integrity_checks:  resq 1
    .integrity_failures: resq 1
endstruc
TraceStatistics_size equ 32

; Trace Buffer Structure
struc TraceBuffer
    .events:       resb (TraceEvent_size * TRACE_MAX_EVENTS)
    .read_index:  resq 1
    .write_index: resq 1
endstruc
TraceBuffer_size equ (TraceEvent_size * TRACE_MAX_EVENTS) + 16

; =============================================================================
; BSS Section - Data Storage
; =============================================================================
section .bss

%define TRACE_MAX_EVENTS 1024
%define TRACE_BUFFER_SIZE (TraceEvent_size * TRACE_MAX_EVENTS)

align 16

; Trace buffer for events
trace_buffer:
    resb TraceBuffer_size

; Statistics
trace_stats:
    resb TraceStatistics_size

; Integrity baseline hashes
integrity_baseline:
    resb (IntegrityEntry_size * 5)  ; 5 regions monitored

; Hardware watchpoints (DR0-DR3)
watchpoints:
    resb Watchpoint_size
    resb Watchpoint_size
    resb Watchpoint_size
    resb Watchpoint_size

; Temporary buffer for current hash
current_hash_buffer:
    resb 32
