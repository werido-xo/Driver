obj-m	:=virtualmem.o

ARCH 	=x86

ifeq ($(ARCH),x86)
	CROSS_COMPILE =
	KDIR	:=/lib/modules/3.13.0-32-generic/build/
else
	CROSS_COMPILE = arm-linux-
	KDIR	:=/home/weirdo-xo/Arm/linux-3.0.8
endif

CC	=$(CROSS_COMPILE)gcc

PWD	:=$(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o* *.mod.c *.symvers *.tmp_versions 