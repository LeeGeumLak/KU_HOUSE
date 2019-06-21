#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

#define DEV_NAME "counter_dev"

#define IOCTL_START_NUM 0x80
#define IOCTL_NUM1 IOCTL_START_NUM+1
#define IOCTL_NUM2 IOCTL_START_NUM+2

#define IOCTL_NUM 'z'
#define COUNTERSTART _IOWR(IOCTL_NUM, IOCTL_NUM1, unsigned long *)
#define COUNTERFINISH _IOWR(IOCTL_NUM, IOCTL_NUM2, unsigned long *)

int dev;

int counter_start(void *people)
{
	int temp;

	dev = open("/dev/counter_dev", O_RDWR);
	temp = ioctl(dev, COUNTERSTART, people);

	return temp;
}

int counter_finish()
{
	int temp;

	temp = ioctl(dev, COUNTERFINISH, NULL);

	close(dev);

	return temp;
}
