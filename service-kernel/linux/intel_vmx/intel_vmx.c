#include <linux/kernel.h>

#include "kernova_kernel.h"

void kernova_intel_vmx_fill_caps(struct kernova_caps *caps)
{
    /*
     * Intel VMX privileged execution is intentionally not implemented in the
     * first Linux driver milestone. The userspace service keeps the VMX model
     * and assembly routines, while the kernel validation path starts on AMD SVM
     * because the available bare-metal validation host is AMD.
     */
    if (!caps->vmx_supported) {
        return;
    }

    caps->backend_flags |= KERNOVA_BACKEND_INTEL_VMX;
}
