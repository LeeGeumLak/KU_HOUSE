#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "MQTTClient.h"

#define DEV_NAME "lamp_dev"

// ioctl
#define IOCTL_START_NUM 0x80
#define IOCTL_NUM1 IOCTL_START_NUM+1
#define IOCTL_NUM2 IOCTL_START_NUM+2

#define LAMP_IOCTL_NUM 'z'
#define LAMPSTART _IOWR(LAMP_IOCTL_NUM, IOCTL_NUM1, unsigned long *)
#define LAMPSTOP _IOWR(LAMP_IOCTL_NUM, IOCTL_NUM2, unsigned long *)

int dev;

int lamp_start(void) {
	dev = open("/dev/lamp_dev", O_RDWR);
	
	return ioctl(dev, LAMPSTART, NULL);
}

int lamp_stop(void) {
	int temp = ioctl(dev, LAMPSTOP, NULL);
	close(dev);

	return temp;
}
