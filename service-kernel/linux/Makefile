obj-m += kernova.o

kernova-y := kernova_main.o
kernova-y += amd_svm/amd_svm.o
kernova-y += intel_vmx/intel_vmx.o

ccflags-y += -I$(src)/include
ccflags-y += -I$(src)/../../shared

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

.PHONY: all clean load load-vmrun unload status logs

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load: all
	sudo insmod kernova.ko

load-vmrun: all
	sudo insmod kernova.ko allow_vmrun=1

unload:
	-sudo rmmod kernova

status:
	ls -l /dev/kernova

logs:
	sudo dmesg | tail -n 80
