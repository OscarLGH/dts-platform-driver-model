#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <sys/mman.h>

#define PLATFORM_MODEL_IOCTL_MAGIC 0x5537
#define PLATFORM_MODEL_IOCTL_SET_IRQFD	_IOW(PLATFORM_MODEL_IOCTL_MAGIC, 2, void *)
#define PLATFORM_MODEL_IOCTL_SET_IRQ	_IOW(PLATFORM_MODEL_IOCTL_MAGIC, 3, void *)
#define PLATFORM_MODEL_IOCTL_ALLOC_DMA	_IOW(PLATFORM_MODEL_IOCTL_MAGIC, 4, void *)
#define PLATFORM_MODEL_IOCTL_FREE_DMA	_IOW(PLATFORM_MODEL_IOCTL_MAGIC, 5, void *)

#define DEVICE_NAME "dts_platform_driver_dev_0"

struct dma_region {
    uint64_t dma_buffer_phys;
	void *unused; //do not touch!
    void *dma_buffer_virt_user;
	size_t dma_buffer_size;
};

int main()
{
    int i;
    int ret;
    struct dma_region dma_regions[8] = {0};
    uint64_t write_buffer[0x100] = {0};
    int device_fd = open(DEVICE_NAME, O_RDWR);
    if (device_fd < 0) {
        printf("open device error!\n");
        return -1;
    }

    //allocate 8 4K dma buffers.
    for (i = 0; i < 8; i++) {
        dma_regions[i].dma_buffer_size = 0x1000;
        ret = ioctl(device_fd, PLATFORM_MODEL_IOCTL_ALLOC_DMA, &dma_regions[i]);
        if (ret < 0) {
            printf("ioctl error, ret = %d\n", ret);
            return -1;
        }
        printf("dma buffer %d: 0x%llx\n", i, dma_regions[i].dma_buffer_phys);
        dma_regions[i].dma_buffer_virt_user =
            mmap(NULL, dma_regions[i].dma_buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, device_fd, dma_regions[i].dma_buffer_phys);
    }

    ret = write(device_fd, write_buffer, 32);
    if (ret < 0) {
        printf("write error, ret = %d\n", ret);
    }

    // free dma buffers.
    for (i = 0; i < 8; i++) {
        ret = ioctl(device_fd, PLATFORM_MODEL_IOCTL_FREE_DMA, &dma_regions[i]);
        if (ret < 0) {
            printf("ioctl error, ret = %d\n", ret);
            return -1;
        }
    }

    close(device_fd);
    return 0;
}

