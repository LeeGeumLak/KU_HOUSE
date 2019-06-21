KDIR := ~/linux-rpi
ARM := ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-
PWD := $(shell pwd)

obj-m := smart_lamp_thread_ioctl.o

default:
	$(MAKE) -C $(KDIR) M=$(PWD) $(ARM) modules
copy:
	scp smart_lamp_thread_ioctl.ko smart_lamp_thread_lib.c mknod.sh smart_lamp_thread_app.c pi@10.10.10.13:~
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) $(ARM) clean
