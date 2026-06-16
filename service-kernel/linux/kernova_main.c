#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include <asm/cpufeature.h>
#include <asm/cpufeatures.h>
#include <asm/processor.h>

#include "kernova_kernel.h"

static struct kernova_driver_state kernova_state;

void kernova_detect_cpu(struct kernova_caps *caps)
{
    u32 eax = 0;
    u32 ebx = 0;
    u32 ecx = 0;
    u32 edx = 0;
    u32 max_leaf = 0;

    memset(caps, 0, sizeof(*caps));
    caps->abi_version = KERNOVA_ABI_VERSION;
    caps->driver_loaded = 1;
    caps->runtime_state = KERNOVA_STATE_CAPS_READY;
    strscpy(caps->driver_name, KERNOVA_DRIVER_NAME, sizeof(caps->driver_name));

    cpuid_count(0, 0, &max_leaf, &ebx, &ecx, &edx);
    memcpy(&caps->cpu_vendor[0], &ebx, sizeof(ebx));
    memcpy(&caps->cpu_vendor[4], &edx, sizeof(edx));
    memcpy(&caps->cpu_vendor[8], &ecx, sizeof(ecx));
    caps->cpu_vendor[12] = '\0';

    if (boot_cpu_has(X86_FEATURE_SVM)) {
        caps->backend_flags |= KERNOVA_BACKEND_AMD_SVM;
        caps->svm_supported = 1;
    }

    if (boot_cpu_has(X86_FEATURE_VMX)) {
        caps->backend_flags |= KERNOVA_BACKEND_INTEL_VMX;
        caps->vmx_supported = 1;
    }

    cpuid_count(0x80000000u, 0, &eax, &ebx, &ecx, &edx);
    if (eax >= 0x8000000Au && caps->svm_supported) {
        cpuid_count(0x8000000Au, 0, &eax, &ebx, &ecx, &edx);
        caps->svm_revision = eax & 0xffu;
        caps->svm_asid_count = ebx;
    }

    if (!caps->backend_flags) {
        caps->backend_flags = KERNOVA_BACKEND_USERSPACE_FALLBACK;
    }
}

static void kernova_refresh_caps_locked(void)
{
    struct kernova_caps current;

    kernova_detect_cpu(&current);
    kernova_state.caps = current;
    kernova_amd_svm_fill_caps(&kernova_state);
    kernova_intel_vmx_fill_caps(&kernova_state.caps);
}

static long kernova_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long ret = 0;

    if (_IOC_TYPE(cmd) != KERNOVA_IOCTL_MAGIC) {
        return -ENOTTY;
    }

    mutex_lock(&kernova_state.lock);
    kernova_refresh_caps_locked();

    switch (cmd) {
    case KERNOVA_IOCTL_QUERY_CAPS:
        if (copy_to_user((void __user *)arg, &kernova_state.caps, sizeof(kernova_state.caps))) {
            ret = -EFAULT;
        }
        break;

    case KERNOVA_IOCTL_INIT_BACKEND: {
        struct kernova_init_request request;

        if (copy_from_user(&request, (void __user *)arg, sizeof(request))) {
            ret = -EFAULT;
            break;
        }

        if (request.backend_flags & KERNOVA_BACKEND_AMD_SVM) {
            ret = kernova_amd_svm_init(&kernova_state);
        } else {
            ret = -ENODEV;
        }
        kernova_refresh_caps_locked();
        break;
    }

    case KERNOVA_IOCTL_CREATE_VM: {
        struct kernova_create_vm_request request;

        if (copy_from_user(&request, (void __user *)arg, sizeof(request))) {
            ret = -EFAULT;
            break;
        }

        if (request.backend_flags & KERNOVA_BACKEND_AMD_SVM) {
            ret = kernova_amd_svm_create_vm(&kernova_state, request.guest_size);
        } else {
            ret = -ENODEV;
        }
        kernova_refresh_caps_locked();
        break;
    }

    case KERNOVA_IOCTL_RUN_VM: {
        struct kernova_run_request request;

        if (copy_from_user(&request, (void __user *)arg, sizeof(request))) {
            ret = -EFAULT;
            break;
        }

        ret = kernova_amd_svm_run(&kernova_state, &request);
        if (copy_to_user((void __user *)arg, &request, sizeof(request))) {
            ret = -EFAULT;
        }
        kernova_refresh_caps_locked();
        break;
    }

    case KERNOVA_IOCTL_DESTROY_VM:
        kernova_amd_svm_destroy(&kernova_state);
        kernova_refresh_caps_locked();
        break;

    default:
        ret = -ENOTTY;
        break;
    }

    mutex_unlock(&kernova_state.lock);
    return ret;
}

static const struct file_operations kernova_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = kernova_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = kernova_ioctl,
#endif
};

static struct miscdevice kernova_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = KERNOVA_DEVICE_NAME,
    .fops = &kernova_fops,
    .mode = 0600,
};

static int __init kernova_module_init(void)
{
    int ret;

    mutex_init(&kernova_state.lock);
    kernova_refresh_caps_locked();

    ret = misc_register(&kernova_misc_device);
    if (ret) {
        pr_err("kernova: failed to register /dev/%s: %d\n", KERNOVA_DEVICE_NAME, ret);
        return ret;
    }

    pr_info("kernova: driver loaded, device /dev/%s created\n", KERNOVA_DEVICE_NAME);
    pr_info("kernova: cpu=%s svm=%u vmx=%u\n",
            kernova_state.caps.cpu_vendor,
            kernova_state.caps.svm_supported,
            kernova_state.caps.vmx_supported);
    return 0;
}

static void __exit kernova_module_exit(void)
{
    mutex_lock(&kernova_state.lock);
    kernova_amd_svm_destroy(&kernova_state);
    mutex_unlock(&kernova_state.lock);

    misc_deregister(&kernova_misc_device);
    pr_info("kernova: driver unloaded\n");
}

module_init(kernova_module_init);
module_exit(kernova_module_exit);

MODULE_LICENSE("MIT");
MODULE_AUTHOR("Thiago Di Faria");
MODULE_DESCRIPTION("Kernova-TEE privileged Linux validation driver");
