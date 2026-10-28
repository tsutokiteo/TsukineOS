#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>

#include "tsukine_abi.h"
#include "mkdel_compat.h"
#include "tsukine_core.h"

/*
 * 请求结构——用户态代理发过来的
 */
struct tsukine_req {
    uint32_t win_ssn;
    uint32_t tsuki_sys;
    uint32_t subcmd;
    uint8_t  payload[256];
    int32_t  result;
} __attribute__((packed));

#define TSUKINE_IOC_MAGIC    'T'
#define TSUKINE_IOC_CALL     _IOWR(TSUKINE_IOC_MAGIC, 1, struct tsukine_req)
#define TSUKINE_IOC_VERSION  _IOR(TSUKINE_IOC_MAGIC, 2, uint32_t)

static int tsukine_do_dispatch(struct tsukine_req *req)
{
    size_t i;

    for (i = 0; i < compat_map_count; i++) {
        if (compat_map[i].win_ssn == req->win_ssn) {
            if (compat_map[i].subcmd == TSUKI_SUB_UNIMPLEMENTED)
                return -ENOSYS;

            pr_debug("tsukine: dispatch ssn=0x%04X -> sys=%u sub=0x%04X\n",
                     req->win_ssn, compat_map[i].tsuki_sys,
                     compat_map[i].subcmd);

            switch (compat_map[i].tsuki_sys) {
            case 551: /* IO */
                return tsukine_io_handler(compat_map[i].subcmd, req->payload);
            case 554: /* Sync */
                return tsukine_sync_handler(compat_map[i].subcmd, req->payload);
            case 558: /* Query/Time */
                return tsukine_query_handler(compat_map[i].subcmd, req->payload);
            default:
                pr_warn("tsukine: unknown tsuki_sys=%u\n", compat_map[i].tsuki_sys);
                return -ENOSYS;
            }
        }
    }
    return -ENOSYS;
}

static long tsukine_dev_ioctl(struct file *file, unsigned int cmd,
                              unsigned long arg)
{
    struct tsukine_req req;
    int ret;

    switch (cmd) {
    case TSUKINE_IOC_VERSION:
        return put_user(1U, (uint32_t __user *)arg);

    case TSUKINE_IOC_CALL:
        if (copy_from_user(&req, (void __user *)arg, sizeof(req)))
            return -EFAULT;

        ret = tsukine_do_dispatch(&req);
        req.result = ret;

        if (copy_to_user((void __user *)arg, &req, sizeof(req)))
            return -EFAULT;
        return 0;

    default:
        return -ENOTTY;
    }
}

static const struct file_operations tsukine_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = tsukine_dev_ioctl,
    .compat_ioctl   = tsukine_dev_ioctl,
};

static struct miscdevice tsukine_miscdev = {
    .minor       = MISC_DYNAMIC_MINOR,
    .name        = "tsukine",
    .fops        = &tsukine_fops,
    .mode        = 0666,
};

/* 桩 handler 实现 */
int tsukine_io_handler(uint32_t subcmd, uint8_t *payload)
{
    pr_info("tsukine: io_handler subcmd=0x%04X (stub)\n", subcmd);
    return -ENOSYS;
}

int tsukine_sync_handler(uint32_t subcmd, uint8_t *payload)
{
    pr_info("tsukine: sync_handler subcmd=0x%04X (stub)\n", subcmd);
    return -ENOSYS;
}

int tsukine_query_handler(uint32_t subcmd, uint8_t *payload)
{
    pr_info("tsukine: query_handler subcmd=0x%04X (stub)\n", subcmd);
    return -ENOSYS;
}

static int __init tsukine_init(void)
{
    int ret;

    ret = misc_register(&tsukine_miscdev);
    if (ret) {
        pr_err("tsukine: misc_register failed: %d\n", ret);
        return ret;
    }

    pr_info("tsukine: /dev/tsukine ready, compat=%zu entries\n",
            compat_map_count);
    return 0;
}

static void __exit tsukine_exit(void)
{
    misc_deregister(&tsukine_miscdev);
    pr_info("tsukine: unloaded\n");
}

module_init(tsukine_init);
module_exit(tsukine_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("TsukineOS");
MODULE_DESCRIPTION("TsukineOS ABI compat dispatch");