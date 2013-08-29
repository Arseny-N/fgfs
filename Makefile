
ifneq ($(KERNELRELEASE),)
	obj-m := gfs.o
	gfs-objs := dir.o inode.o file.o sysfs.o
else
#	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	KERNELDIR ?= ../../kernels/linux-3.10.5
	PWD := $(shell pwd)
endif

default:
	@ echo -e \\e[44m
	@./utils/fill_line.awk
	@echo -e \\e[0m
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	cp *.ko ../ZeroModules
	@echo `date`
fs:
	./utils/mkgfs -f ../vm/initramfs/gfsRoot.img
.ONESHELL:
rebuild-fs:
	@ cd utils
	gcc -o mkgfs mkgfs.c
