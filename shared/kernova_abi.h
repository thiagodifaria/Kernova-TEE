#ifndef KERNOVA_ABI_H
#define KERNOVA_ABI_H

#include <stdint.h>

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define KERNOVA_ABI_VERSION 0x00010000u
#define KERNOVA_DEVICE_NAME "kernova"
#define KERNOVA_DEVICE_PATH "/dev/kernova"
#define KERNOVA_DRIVER_NAME "kernova-linux"

#define KERNOVA_BACKEND_AMD_SVM (1u << 0)
#define KERNOVA_BACKEND_INTEL_VMX (1u << 1)
#define KERNOVA_BACKEND_USERSPACE_FALLBACK (1u << 2)

#define KERNOVA_RUN_ALLOW_PRIVILEGED (1u << 0)

enum kernova_runtime_state {
    KERNOVA_STATE_UNINITIALIZED = 0,
    KERNOVA_STATE_CAPS_READY = 1,
    KERNOVA_STATE_BACKEND_INITIALIZED = 2,
    KERNOVA_STATE_VM_CREATED = 3,
    KERNOVA_STATE_LAUNCH_BLOCKED = 4,
    KERNOVA_STATE_VMEXIT_RECEIVED = 5,
    KERNOVA_STATE_ERROR = 6,
};

struct kernova_caps {
    uint32_t abi_version;
    uint32_t backend_flags;
    uint32_t runtime_state;
    uint32_t reserved0;

    char driver_name[32];
    char cpu_vendor[13];
    char reserved1[3];

    uint32_t svm_revision;
    uint32_t svm_asid_count;

    uint8_t driver_loaded;
    uint8_t svm_supported;
    uint8_t vmx_supported;
    uint8_t svm_initialized;
    uint8_t vm_created;
    uint8_t vmrun_enabled;
    uint8_t last_vmexit_valid;
    uint8_t reserved2;

    uint64_t last_exit_code;
    uint64_t last_exit_info1;
    uint64_t last_exit_info2;
};

struct kernova_init_request {
    uint32_t backend_flags;
    uint32_t reserved;
};

struct kernova_create_vm_request {
    uint32_t backend_flags;
    uint32_t guest_size;
};

struct kernova_run_request {
    uint32_t flags;
    uint32_t reserved;
    uint32_t runtime_state;
    int32_t status;
    uint64_t exit_code;
    uint64_t exit_info1;
    uint64_t exit_info2;
};

#define KERNOVA_IOCTL_MAGIC 'K'
#define KERNOVA_IOCTL_QUERY_CAPS _IOR(KERNOVA_IOCTL_MAGIC, 0x01, struct kernova_caps)
#define KERNOVA_IOCTL_INIT_BACKEND _IOW(KERNOVA_IOCTL_MAGIC, 0x02, struct kernova_init_request)
#define KERNOVA_IOCTL_CREATE_VM _IOW(KERNOVA_IOCTL_MAGIC, 0x03, struct kernova_create_vm_request)
#define KERNOVA_IOCTL_RUN_VM _IOWR(KERNOVA_IOCTL_MAGIC, 0x04, struct kernova_run_request)
#define KERNOVA_IOCTL_DESTROY_VM _IO(KERNOVA_IOCTL_MAGIC, 0x05)

#ifdef __cplusplus
}
#endif

#endif // KERNOVA_ABI_H
