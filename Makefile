obj-m += dts-platform-driver.o

all:
	make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -C $(KERNELDIR) M=$(PWD) modules KCFLAGS="-fno-asynchronous-unwind-tables -fno-unwind-tables"
	riscv64-linux-gnu-gcc dts-platform-user.c -o dts-platform-user.out
clean:
	make -C $(KERNELDIR) M=$(PWD) clean