#include <linux/gfp.h>
#include <linux/bits.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/string.h>

#include <asm/io.h>
#include <asm/msr.h>
#include <asm/processor.h>

#include "kernova_kernel.h"

#define KERNOVA_MSR_EFER 0xc0000080u
#define KERNOVA_MSR_VM_CR 0xc0010114u
#define KERNOVA_MSR_VM_HSAVE_PA 0xc0010117u

#define KERNOVA_EFER_SVME BIT_ULL(12)
#define KERNOVA_VM_CR_SVMDIS BIT_ULL(4)

#define KERNOVA_VMCB_SIZE 4096u
#define KERNOVA_GUEST_PAGE_SIZE 4096u

#define VMCB_CONTROL_INTERCEPT 0x00c
#define VMCB_CONTROL_ASID 0x058
#define VMCB_CONTROL_TLB_CONTROL 0x05c
#define VMCB_CONTROL_EXIT_CODE 0x070
#define VMCB_CONTROL_EXIT_INFO1 0x078
#define VMCB_CONTROL_EXIT_INFO2 0x080
#define VMCB_CONTROL_N_RIP 0x0c8

#define VMCB_SAVE_ES 0x400
#define VMCB_SAVE_CS 0x410
#define VMCB_SAVE_SS 0x420
#define VMCB_SAVE_DS 0x430
#define VMCB_SAVE_FS 0x440
#define VMCB_SAVE_GS 0x450
#define VMCB_SAVE_GDTR 0x460
#define VMCB_SAVE_LDTR 0x470
#define VMCB_SAVE_IDTR 0x480
#define VMCB_SAVE_TR 0x490
#define VMCB_SAVE_EFER 0x4d0
#define VMCB_SAVE_CR4 0x548
#define VMCB_SAVE_CR3 0x550
#define VMCB_SAVE_CR0 0x558
#define VMCB_SAVE_DR7 0x560
#define VMCB_SAVE_DR6 0x568
#define VMCB_SAVE_RFLAGS 0x570
#define VMCB_SAVE_RIP 0x578
#define VMCB_SAVE_RSP 0x5d8
#define VMCB_SAVE_RAX 0x5f8

#define SVM_INTERCEPT_HLT 24

static bool kernova_allow_vmrun;
module_param_named(allow_vmrun, kernova_allow_vmrun, bool, 0600);
MODULE_PARM_DESC(allow_vmrun, "Allow experimental AMD VMRUN execution from the Kernova driver");

struct kernova_segment_state {
    u16 selector;
    u16 attrib;
    u32 limit;
    u64 base;
} __packed;

static void vmcb_write_u8(void *vmcb, u32 offset, u8 value)
{
    *((u8 *)vmcb + offset) = value;
}

static void vmcb_write_u32(void *vmcb, u32 offset, u32 value)
{
    *((u32 *)((u8 *)vmcb + offset)) = value;
}

static void vmcb_write_u64(void *vmcb, u32 offset, u64 value)
{
    *((u64 *)((u8 *)vmcb + offset)) = value;
}

static u64 vmcb_read_u64(void *vmcb, u32 offset)
{
    return *((u64 *)((u8 *)vmcb + offset));
}

static void vmcb_write_segment(void *vmcb, u32 offset, u16 selector, u16 attrib, u32 limit, u64 base)
{
    struct kernova_segment_state segment;

    segment.selector = selector;
    segment.attrib = attrib;
    segment.limit = limit;
    segment.base = base;
    memcpy((u8 *)vmcb + offset, &segment, sizeof(segment));
}

static int kernova_alloc_page(void **page, phys_addr_t *pa)
{
    void *memory;

    memory = (void *)__get_free_page(GFP_KERNEL | __GFP_ZERO);
    if (!memory) {
        return -ENOMEM;
    }

    *page = memory;
    *pa = virt_to_phys(memory);
    return 0;
}

static void kernova_free_page(void **page)
{
    if (*page) {
        free_page((unsigned long)*page);
        *page = NULL;
    }
}

static void kernova_prepare_guest_page(struct kernova_amd_svm_context *ctx)
{
    u8 *guest = ctx->guest_page;

    memset(guest, 0xf4, KERNOVA_GUEST_PAGE_SIZE);
    guest[0] = 0x90;
    guest[1] = 0x90;
    guest[2] = 0xf4;
}

static void kernova_prepare_vmcb(struct kernova_amd_svm_context *ctx)
{
    u64 intercepts = BIT_ULL(SVM_INTERCEPT_HLT);

    memset(ctx->vmcb, 0, KERNOVA_VMCB_SIZE);
    vmcb_write_u64(ctx->vmcb, VMCB_CONTROL_INTERCEPT, intercepts);
    vmcb_write_u32(ctx->vmcb, VMCB_CONTROL_ASID, 1);
    vmcb_write_u8(ctx->vmcb, VMCB_CONTROL_TLB_CONTROL, 1);

    vmcb_write_segment(ctx->vmcb, VMCB_SAVE_ES, 0, 0x93, 0xffff, 0);
    vmcb_write_segment(ctx->vmcb, VMCB_SAVE_CS, 0, 0x9b, 0xffff, 0);
    vmcb_write_segment(ctx->vmcb, VMCB_SAVE_SS, 0, 0x93, 0xffff, 0);
    vmcb_write_segment(ctx->vmcb, VMCB_SAVE_DS, 0, 0x93, 0xffff, 0);
    vmcb_write_segment(ctx->vmcb, VMCB_SAVE_FS, 0, 0x93, 0xffff, 0);
    vmcb_write_segment(ctx->vmcb, VMCB_SAVE_GS, 0, 0x93, 0xffff, 0);
    vmcb_write_segment(ctx->vmcb, VMCB_SAVE_GDTR, 0, 0x82, 0xffff, 0);
    vmcb_write_segment(ctx->vmcb, VMCB_SAVE_LDTR, 0, 0x82, 0xffff, 0);
    vmcb_write_segment(ctx->vmcb, VMCB_SAVE_IDTR, 0, 0x82, 0xffff, 0);
    vmcb_write_segment(ctx->vmcb, VMCB_SAVE_TR, 0, 0x8b, 0xffff, 0);

    vmcb_write_u64(ctx->vmcb, VMCB_SAVE_EFER, 0);
    vmcb_write_u64(ctx->vmcb, VMCB_SAVE_CR0, 0x10);
    vmcb_write_u64(ctx->vmcb, VMCB_SAVE_CR3, 0);
    vmcb_write_u64(ctx->vmcb, VMCB_SAVE_CR4, 0);
    vmcb_write_u64(ctx->vmcb, VMCB_SAVE_DR6, 0xffff0ff0);
    vmcb_write_u64(ctx->vmcb, VMCB_SAVE_DR7, 0x400);
    vmcb_write_u64(ctx->vmcb, VMCB_SAVE_RFLAGS, 0x2);
    vmcb_write_u64(ctx->vmcb, VMCB_SAVE_RIP, ctx->guest_pa);
    vmcb_write_u64(ctx->vmcb, VMCB_SAVE_RSP, ctx->guest_pa + KERNOVA_GUEST_PAGE_SIZE - 16);
    vmcb_write_u64(ctx->vmcb, VMCB_SAVE_RAX, 0);
    vmcb_write_u64(ctx->vmcb, VMCB_CONTROL_N_RIP, ctx->guest_pa + 3);
}

void kernova_amd_svm_fill_caps(struct kernova_driver_state *state)
{
    state->caps.svm_initialized = state->svm.initialized ? 1 : 0;
    state->caps.vm_created = state->svm.vm_created ? 1 : 0;
    state->caps.vmrun_enabled = kernova_allow_vmrun ? 1 : 0;
    state->caps.last_vmexit_valid = state->svm.last_exit_code ? 1 : 0;
    state->caps.last_exit_code = state->svm.last_exit_code;
    state->caps.last_exit_info1 = state->svm.last_exit_info1;
    state->caps.last_exit_info2 = state->svm.last_exit_info2;

    if (state->svm.vm_created) {
        state->caps.runtime_state = KERNOVA_STATE_VM_CREATED;
    } else if (state->svm.initialized) {
        state->caps.runtime_state = KERNOVA_STATE_BACKEND_INITIALIZED;
    }
}

int kernova_amd_svm_init(struct kernova_driver_state *state)
{
    u64 efer;
    u64 vm_cr;
    int ret;

    if (!state->caps.svm_supported) {
        return -ENODEV;
    }

    rdmsrl(KERNOVA_MSR_VM_CR, vm_cr);
    if (vm_cr & KERNOVA_VM_CR_SVMDIS) {
        pr_err("kernova: AMD SVM disabled by VM_CR.SVMDIS\n");
        return -EOPNOTSUPP;
    }

    if (state->svm.initialized) {
        return 0;
    }

    ret = kernova_alloc_page(&state->svm.host_save_area, &state->svm.host_save_pa);
    if (ret) {
        return ret;
    }

    rdmsrl(KERNOVA_MSR_EFER, state->svm.original_efer);
    state->svm.efer_saved = true;
    rdmsrl(KERNOVA_MSR_VM_HSAVE_PA, state->svm.original_vm_hsave_pa);
    state->svm.hsave_saved = true;

    efer = state->svm.original_efer | KERNOVA_EFER_SVME;
    wrmsrl(KERNOVA_MSR_EFER, efer);
    wrmsrl(KERNOVA_MSR_VM_HSAVE_PA, state->svm.host_save_pa);

    state->svm.initialized = true;
    pr_info("kernova: AMD SVM initialized, host save PA=%pa\n", &state->svm.host_save_pa);
    return 0;
}

int kernova_amd_svm_create_vm(struct kernova_driver_state *state, u32 guest_size)
{
    int ret;

    if (!state->svm.initialized) {
        return -EINVAL;
    }

    if (guest_size != 0 && guest_size > KERNOVA_GUEST_PAGE_SIZE) {
        return -EINVAL;
    }

    if (!state->svm.vmcb) {
        ret = kernova_alloc_page(&state->svm.vmcb, &state->svm.vmcb_pa);
        if (ret) {
            return ret;
        }
    }

    if (!state->svm.guest_page) {
        ret = kernova_alloc_page(&state->svm.guest_page, &state->svm.guest_pa);
        if (ret) {
            kernova_free_page(&state->svm.vmcb);
            return ret;
        }
    }

    kernova_prepare_guest_page(&state->svm);
    kernova_prepare_vmcb(&state->svm);
    state->svm.vm_created = true;

    pr_info("kernova: AMD SVM VM prepared, vmcb=%pa guest=%pa\n",
            &state->svm.vmcb_pa,
            &state->svm.guest_pa);
    return 0;
}

int kernova_amd_svm_run(struct kernova_driver_state *state, struct kernova_run_request *request)
{
    unsigned long irq_flags;
    u64 vmcb_pa;

    request->status = 0;
    request->runtime_state = KERNOVA_STATE_UNINITIALIZED;
    request->exit_code = 0;
    request->exit_info1 = 0;
    request->exit_info2 = 0;

    if (!state->svm.initialized || !state->svm.vm_created) {
        request->status = -EINVAL;
        request->runtime_state = KERNOVA_STATE_ERROR;
        return -EINVAL;
    }

    if (!kernova_allow_vmrun || !(request->flags & KERNOVA_RUN_ALLOW_PRIVILEGED)) {
        request->status = -EPERM;
        request->runtime_state = KERNOVA_STATE_LAUNCH_BLOCKED;
        return -EPERM;
    }

    vmcb_pa = state->svm.vmcb_pa;

    local_irq_save(irq_flags);
    asm volatile(".byte 0x0f, 0x01, 0xd8"
                 : "+a"(vmcb_pa)
                 :
                 : "memory", "cc");
    local_irq_restore(irq_flags);

    state->svm.last_exit_code = vmcb_read_u64(state->svm.vmcb, VMCB_CONTROL_EXIT_CODE);
    state->svm.last_exit_info1 = vmcb_read_u64(state->svm.vmcb, VMCB_CONTROL_EXIT_INFO1);
    state->svm.last_exit_info2 = vmcb_read_u64(state->svm.vmcb, VMCB_CONTROL_EXIT_INFO2);

    request->runtime_state = KERNOVA_STATE_VMEXIT_RECEIVED;
    request->exit_code = state->svm.last_exit_code;
    request->exit_info1 = state->svm.last_exit_info1;
    request->exit_info2 = state->svm.last_exit_info2;

    pr_info("kernova: AMD VMRUN returned VMEXIT code=%llu info1=%llu info2=%llu\n",
            state->svm.last_exit_code,
            state->svm.last_exit_info1,
            state->svm.last_exit_info2);
    return 0;
}

void kernova_amd_svm_destroy(struct kernova_driver_state *state)
{
    if (state->svm.hsave_saved) {
        wrmsrl(KERNOVA_MSR_VM_HSAVE_PA, state->svm.original_vm_hsave_pa);
    }

    if (state->svm.efer_saved) {
        wrmsrl(KERNOVA_MSR_EFER, state->svm.original_efer);
    }

    kernova_free_page(&state->svm.guest_page);
    kernova_free_page(&state->svm.vmcb);
    kernova_free_page(&state->svm.host_save_area);
    memset(&state->svm, 0, sizeof(state->svm));
}
