KPROJ=cachegrab

# Set this variable to the location of the Linux kernel
KERNEL_DIR=

# Set this variable to the location of the aarch64-linux-android- binaries
CROSS_COMPILE_DIR=

ifndef KERNEL_DIR
$(error Set KERNEL_DIR variable in Makefile)
endif

ifndef CROSS_COMPILE_DIR
$(error Set CROSS_COMPILE_DIR variable in Makefile)
endif

obj-m += cachegrab.o
cachegrab-objs := src/cachegrab.o
cachegrab-objs += src/cachegrab_ioctl.o
cachegrab-objs += src/scope.o src/probes.o
cachegrab-objs += src/probe_generic.o
cachegrab-objs += src/probe_l1d.o src/probe_l1i.o src/probe_btb.o
cachegrab-objs += src/memory.o

$(KPROJ):
	make ARCH=arm64 CFLAGS_MODULE=-fno-pic \
		CROSS_COMPILE=$(CROSS_COMPILE_DIR)/aarch64-linux-android- \
		-C $(KERNEL_DIR) SUBDIRS=$(PWD) modules

deploy: $(KPROJ)
	adb shell su -c "rmmod $(KPROJ)"
	adb push $(KPROJ).ko /data/local/tmp/$(KPROJ).ko
	adb shell su -c "insmod /data/local/tmp/$(KPROJ).ko"

style:
	indent -linux src/*.c src/*.h

clean:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) clean
	rm -rf *~ src/*~
