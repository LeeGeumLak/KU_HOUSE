KDIR := ~/linux-rpi
ARM := ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-
PWD := $(shell pwd)

obj-m := smart_counter1.o

default:
	$(MAKE) -C $(KDIR) M=$(PWD) $(ARM) modules

copy:
	scp smart_counter1.ko smart_counter1_app.c counter_app2.c smart_counter1_lib.c mknod.sh pi@10.10.10.13:~

clean:
	$(MAKE) -C $(KDIR) M=$(PWD_ $(ARM) clean
	
