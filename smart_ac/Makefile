KDIR := ~/linux-rpi
ARM := ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-
PWD := $(shell pwd)

obj-m := air.o

default:
	$(MAKE) -C $(KDIR) M=$(PWD) $(ARM) modules

copy:
	scp air.ko air_app.c air_lib.c mknod.sh pi@10.10.10.13:~

clean:
	$(MAKE) -C $(KDIR) M=$(PWD_ $(ARM) clean
	
