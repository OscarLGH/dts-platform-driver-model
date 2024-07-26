#include <stdio.h>
#include <stdlib.h>
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

#define DEVICE_NAME "/dev/soc_dev_0"

typedef struct dma_region {
    uint64_t dma_buffer_phys;
	void *unused; //do not touch!
    void *dma_buffer_virt_user;
	size_t dma_buffer_size;
} dma_region;

dma_region * allocate_dma(int device_fd, size_t dma_size)
{
    int ret;
    dma_region *dma_desc = malloc(sizeof(*dma_desc));
    dma_desc->dma_buffer_size = dma_size;
    ret = ioctl(device_fd, PLATFORM_MODEL_IOCTL_ALLOC_DMA, dma_desc);
    if (ret < 0) {
        printf("allocate dma error, ret = %d\n", ret);
        free(dma_desc);
        return NULL;
    }
    dma_desc->dma_buffer_virt_user =
            mmap(NULL, dma_desc->dma_buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, device_fd, dma_desc->dma_buffer_phys);
    if (!dma_desc->dma_buffer_virt_user) {
        ioctl(device_fd, PLATFORM_MODEL_IOCTL_FREE_DMA, dma_desc);
        printf("mmap dma error, ret = %d\n", ret);
        free(dma_desc);
        return NULL;
    }

    return dma_desc;
}

int free_dma(int device_fd, dma_region * dma_desc)
{
    if (!dma_desc)
        return -1;

    munmap(dma_desc->dma_buffer_virt_user, dma_desc->dma_buffer_size);
    ioctl(device_fd, PLATFORM_MODEL_IOCTL_FREE_DMA, dma_desc);
    return 0;
}

int send_command(int device_fd, uint64_t *cmd, int size)
{
    int ret = 0;
    ret = write(device_fd, cmd, size);
    if (ret < 0) {
        printf("write error, ret = %d\n", ret);
    }
    return ret;
}

int main()
{
    int i;
    int ret;
    struct dma_region *dma_regions[8] = {0};
    uint64_t *dma_region_ptr;
    uint64_t cmd[0x100] = {0};
    int device_fd = open(DEVICE_NAME, O_RDWR);
    if (device_fd < 0) {
        printf("open device error!\n");
        return -1;
    }

    //allocate 8 4K dma buffers.
    for (i = 0; i < 8; i++) {
        dma_regions[i] = allocate_dma(device_fd, 0x1000);
        if (!dma_regions[i]) {
            printf("allocate dma error!\n");
            return -1;
        }
        printf("allocated dma:phys addr = 0x%llx virt addr = 0x%llx size = %d\n",
            dma_regions[i]->dma_buffer_phys,
            dma_regions[i]->dma_buffer_virt_user,
            dma_regions[i]->dma_buffer_size
        );
    }

    dma_region_ptr = dma_regions[0]->dma_buffer_virt_user;

    dma_region_ptr[0] = 0x55aaaa55;
    dma_region_ptr[1] = 0xaa5555aa;
    dma_region_ptr[2] = 0x12345678;
    dma_region_ptr[3] = 0x87654321;


    cmd[0] = 0x0;
    cmd[1] = 0x1;
    cmd[2] = 0x2;
    cmd[3] = 0x3;

    ret = send_command(device_fd, cmd, sizeof(cmd[0]) * 4);
    if (ret < 0) {
        printf("write error, ret = %d\n", ret);
    }

    // free dma buffers.
    for (i = 0; i < 8; i++) {
        ret = free_dma(device_fd, dma_regions[i]);
        if (ret < 0) {
            printf("free dma error, ret = %d\n", ret);
            return -1;
        }
    }

    close(device_fd);
    return 0;
}

