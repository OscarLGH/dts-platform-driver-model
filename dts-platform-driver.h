#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <asm/cacheflush.h>

#include <linux/dma-mapping.h>
#include <linux/of.h>
#include <linux/of_device.h>


#include <linux/eventfd.h>
#include <linux/workqueue.h>

#include <linux/list.h>

struct mmio_reg {
    u64 phys;
    size_t size;
    void __iomem *cpu_ptr;
};

struct dma_region {
    dma_addr_t dma_buffer_phys;
    void *dma_buffer_virt_kernel;
    void *dma_buffer_virt_user;
    size_t dma_buffer_size;
};

struct dma_region_node {
    struct dma_region* region;
    struct list_head list;
};

struct dts_platform_driver_model {

    u32 device_idx; //for multiple devices.
	spinlock_t lock;
	u32 irq_cnt;
	u32 reserved;

    struct mmio_reg reg;
	/* DMA buffer */
	struct dma_region dma_block[8];

	int irq_count;
    int irq;

	/* for eventfd */
	struct eventfd_ctx *efd_ctx;

	/* work queue for irq buttom half */
	struct work_struct irq_wq;

	/* for char dev file */
	struct cdev cdev;
	dev_t dev;

    struct platform_device *pdev;

	/* for proc dev file */
	struct proc_dir_entry *device_proc_entry;

    struct list_head dma_regions_list;
};

/* basic ioctls */
#define PLATFORM_MODEL_IOCTL_MAGIC 0x5537
#define PLATFORM_MODEL_IOCTL_SET_IRQFD	_IOW(PLATFORM_MODEL_IOCTL_MAGIC, 2, void *)
#define PLATFORM_MODEL_IOCTL_SET_IRQ	_IOW(PLATFORM_MODEL_IOCTL_MAGIC, 3, void *)
#define PLATFORM_MODEL_IOCTL_ALLOC_DMA	_IOW(PLATFORM_MODEL_IOCTL_MAGIC, 4, void *)
#define PLATFORM_MODEL_IOCTL_FREE_DMA	_IOW(PLATFORM_MODEL_IOCTL_MAGIC, 5, void *)