obj-m += dts-platform-driver.o

all:
	make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -C $(KERNELDIR) M=$(PWD) modules KCFLAGS="-fno-asynchronous-unwind-tables -fno-unwind-tables"

clean:
	make -C $(KERNELDIR) M=$(PWD) clean