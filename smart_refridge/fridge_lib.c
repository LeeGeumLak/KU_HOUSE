#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

#define DEV_NAME "fridge_dev"

#define IOCTL_START_NUM 0x80
#define IOCTL_NUM1 IOCTL_START_NUM+1
#define IOCTL_NUM2 IOCTL_START_NUM+2

#define IOCTL_NUM 'z'
#define FRIDGESTART _IOWR(IOCTL_NUM, IOCTL_NUM1, unsigned long *)
#define FRIDGEFINISH _IOWR(IOCTL_NUM, IOCTL_NUM2, unsigned long *)

int dev;

int fridge_start(){
	int temp;
	printf("ddddddddddd\n");
	dev = open("/dev/fridge_dev", O_RDWR);
	temp = ioctl(dev, FRIDGESTART, NULL);
	
	return temp;
}

int fridge_finish(){
	int temp;
	printf("fffffffffffff\n");
	temp = ioctl(dev, FRIDGEFINISH, NULL);
	
	close(dev);
	
	return temp;
}
