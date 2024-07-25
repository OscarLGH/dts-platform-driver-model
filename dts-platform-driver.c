#include "dts-platform-driver.h"

#define MY_MMIO_OFFSET 0x0  // Offset to write data

struct class *dts_platform_driver_model_class;
u64 device_cnt = 0;

static irqreturn_t dts_platform_driver_irq(int irq, void *data)
{
	irqreturn_t result = IRQ_HANDLED;//IRQ_NONE
	struct dts_platform_driver_model *drv_data = data;
	drv_data->irq_cnt++;
	printk("IRQ:%d\n", irq);

	if (drv_data->efd_ctx)
		eventfd_signal(drv_data->efd_ctx);

	schedule_work(&drv_data->irq_wq);
	return result;
}

static irqreturn_t dts_platform_driver_irq_check(int irq, void *data)
{
	struct dts_platform_driver_model *drv_data = data;
	printk("IRQ:%d\n", irq);
	printk("IRQ cnt:%d\n", drv_data->irq_cnt);
	return IRQ_WAKE_THREAD;//IRQ_NONE
}

static int dts_platform_driver_model_char_open(struct inode *inode, struct file *file)
{
	struct dts_platform_driver_model *pdev;
	pdev = container_of(inode->i_cdev, struct dts_platform_driver_model, cdev);
	file->private_data = pdev;

	return 0;
}

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
static loff_t dts_platform_driver_model_char_lseek(struct file *file, loff_t offset, int origin)
{
	int index;
	loff_t retval = 0;
	struct dts_platform_driver_model *dev = file->private_data;
	//sscanf(filp->f_path.dentry->d_iname, "bar%d", &index);

	switch (origin) {
		case SEEK_SET:
			file->f_pos = offset;
			break;
		case SEEK_END:
			//retval = dev->regs[index].size;
			file->f_pos = retval;
			break;
		default:
			break;
	}
	return retval;
}
static ssize_t dts_platform_driver_model_char_read(struct file *file, char __user *buf,
		size_t count, loff_t *pos)
{
	struct dts_platform_driver_model *dev = file->private_data;
	return 0;
}

static ssize_t dts_platform_driver_model_char_write(struct file *file, const char __user *buf,
                size_t count, loff_t *pos)
{
	struct dts_platform_driver_model *dev = file->private_data;
    u64 data[4];
    int i;
    copy_from_user(data, buf, 32);
    for (i = 0; i < count / 8; i++)
        iowrite64(data[i], dev->reg.cpu_ptr);
	return 0;
}

static long dts_platform_driver_model_char_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct dts_platform_driver_model *dev = file->private_data;
	int ret = 0;
    struct dma_region dma_usr_region;

	switch (cmd) {
		case PLATFORM_MODEL_IOCTL_SET_IRQFD:
			dev->efd_ctx = eventfd_ctx_fdget(arg);
			ret = 0;
			break;
		case PLATFORM_MODEL_IOCTL_SET_IRQ:
			if (arg) {
			/* enable IRQ */

			} else {
			/* disable IRQ */

			}
			ret = 0;
			break;
        case PLATFORM_MODEL_IOCTL_ALLOC_DMA:
            copy_from_user(&dma_usr_region, (void * __user)arg, sizeof(dma_usr_region));
            dma_usr_region.dma_buffer_virt_kernel =
                dma_alloc_coherent(&dev->pdev->dev, dma_usr_region.dma_buffer_size, &dma_usr_region.dma_buffer_phys, GFP_KERNEL);
            if (!dma_usr_region.dma_buffer_virt_kernel) {
                dev_err(&dev->pdev->dev, "Failed to allocate DMA buffer.");
                return -ENOMEM;
            }
            copy_to_user((void * __user)arg, &dma_usr_region, sizeof(dma_usr_region));
            ret = 0;
            break;
        case PLATFORM_MODEL_IOCTL_FREE_DMA:
            copy_from_user(&dma_usr_region, (void * __user)arg, sizeof(dma_usr_region));
            dma_free_coherent(&dev->pdev->dev, dma_usr_region.dma_buffer_size, dma_usr_region.dma_buffer_virt_kernel, dma_usr_region.dma_buffer_phys);
            ret = 0;
		default:
			ret = 0;
			break;
	}

	return ret;
}

static int dts_platform_driver_model_char_mmap(struct file *file, struct vm_area_struct *vma)
{
	size_t vma_size = vma->vm_end - vma->vm_start;
	int i = 0;
	long current_offset = 0;
	long current_size = 0;
	struct dts_platform_driver_model *dev = file->private_data;

	pgprot_noncached(vma->vm_page_prot);
	while (current_size < vma_size) {

		if (remap_pfn_range(vma,
			vma->vm_start,
			vma->vm_pgoff,
			vma_size,
			vma->vm_page_prot)) {
			return -EAGAIN;
		}

		current_size += vma_size;
	}

	return 0;
}

static const struct file_operations dts_platform_driver_model_char_fops = {
	.owner = THIS_MODULE,
	.open = dts_platform_driver_model_char_open,
	.write = dts_platform_driver_model_char_write,
	.read = dts_platform_driver_model_char_read,
	.mmap = dts_platform_driver_model_char_mmap,
	.unlocked_ioctl = dts_platform_driver_model_char_ioctl,
	.llseek = dts_platform_driver_model_char_lseek,
};

int dts_platform_driver_model_device_fd_create(struct dts_platform_driver_model *pdm_dev)
{
	int major, minor;
	int ret;
	char buffer[256] = {0};
	sprintf(buffer,
		"dts_platform_driver_dev_%d",
		pdm_dev->device_idx
	);
	alloc_chrdev_region(&pdm_dev->dev, 0, 255, "dts_platform_driver_model");

	cdev_init(&pdm_dev->cdev, &dts_platform_driver_model_char_fops);
	pdm_dev->cdev.owner = THIS_MODULE;
	ret = cdev_add(&pdm_dev->cdev, pdm_dev->dev, 1);

	device_create(dts_platform_driver_model_class, NULL, pdm_dev->dev, NULL, buffer);
	printk("add char file. (%d %d))\n", MAJOR(pdm_dev->dev), MINOR(pdm_dev->dev), buffer);

	return 0;
}

int dts_platform_driver_model_device_fd_destory(struct dts_platform_driver_model *pdm_dev)
{
	device_destroy(dts_platform_driver_model_class, pdm_dev->dev);
	return 0;
}

void irq_work_queue_func(struct work_struct *wq)
{
	struct dts_platform_driver_model *drv_data = container_of(wq, struct dts_platform_driver_model, irq_wq);
	printk("work queue wakeup.\n");
}


static int dts_platform_driver_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct dts_platform_driver_model *data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->device_idx = device_cnt++;
    data->pdev = pdev;
    struct resource *res;
    void __iomem *base;
    int i;

    // Get the MMIO region from device tree
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "Failed to get MMIO resource\n");
        return -ENODEV;
    }

    // Request and map the MMIO region
    base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(base)) {
        dev_err(&pdev->dev, "Failed to map MMIO resource\n");
        return PTR_ERR(base);
    }

    data->reg.cpu_ptr = base;
    data->reg.size = PAGE_SIZE;
    data->reg.phys = res->start;

    data->irq = platform_get_irq(pdev, 0);
    printk("irq:%d.\n", data->irq);

    ret = devm_request_irq(&pdev->dev, data->irq, dts_platform_driver_irq, 0, "", data);
    if (ret) {
        dev_err(&pdev->dev, "Failed to request IRQ %d %d\n", data->irq, ret);
        return ret;
    }

    dts_platform_driver_model_device_fd_create(data);

    dev_set_drvdata(&pdev->dev, data);
    return 0;
}

static int dts_platform_driver_remove(struct platform_device *pdev)
{
    // Unmap and cleanup is automatically handled by devm_ioremap_resource
    return 0;
}

static const struct of_device_id dts_platform_driver_of_match[] = {
    { .compatible = "dts_platform_driver,mmio", },
    {},
};

MODULE_DEVICE_TABLE(of, dts_platform_driver_of_match);

static struct platform_driver dts_platform_driver_driver = {
    .probe  = dts_platform_driver_probe,
    .remove = dts_platform_driver_remove,
    .driver = {
        .name = "dts_platform_driver",
        .of_match_table = dts_platform_driver_of_match,
    },
};

module_platform_driver(dts_platform_driver_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple MMIO device driver");
