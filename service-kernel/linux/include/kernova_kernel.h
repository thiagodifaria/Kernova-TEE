#ifndef KERNOVA_KERNEL_H
#define KERNOVA_KERNEL_H

#include <linux/mutex.h>
#include <linux/types.h>

#include "kernova_abi.h"

struct kernova_amd_svm_context {
    void *vmcb;
    void *host_save_area;
    void *guest_page;
    phys_addr_t vmcb_pa;
    phys_addr_t host_save_pa;
    phys_addr_t guest_pa;
    u64 original_efer;
    u64 original_vm_hsave_pa;
    bool efer_saved;
    bool hsave_saved;
    bool initialized;
    bool vm_created;
    u64 last_exit_code;
    u64 last_exit_info1;
    u64 last_exit_info2;
};

struct kernova_driver_state {
    struct mutex lock;
    struct kernova_caps caps;
    struct kernova_amd_svm_context svm;
};

void kernova_detect_cpu(struct kernova_caps *caps);

int kernova_amd_svm_init(struct kernova_driver_state *state);
int kernova_amd_svm_create_vm(struct kernova_driver_state *state, u32 guest_size);
int kernova_amd_svm_run(struct kernova_driver_state *state, struct kernova_run_request *request);
void kernova_amd_svm_destroy(struct kernova_driver_state *state);
void kernova_amd_svm_fill_caps(struct kernova_driver_state *state);

void kernova_intel_vmx_fill_caps(struct kernova_caps *caps);

#endif // KERNOVA_KERNEL_H
