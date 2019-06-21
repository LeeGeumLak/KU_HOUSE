#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

#define DEV_NAME "air_dev"

#define IOCTL_START_NUM 0x80
#define IOCTL_NUM1 IOCTL_START_NUM+1
#define IOCTL_NUM2 IOCTL_START_NUM+2

#define IOCTL_NUM 'z'
#define AIRSTART _IOWR(IOCTL_NUM, IOCTL_NUM1, unsigned long *)
#define AIRFINISH _IOWR(IOCTL_NUM, IOCTL_NUM2, unsigned long *)

int dev;

int air_start()
{
	int temp;

	dev = open("/dev/air_dev", O_RDWR);
	temp = ioctl(dev, AIRSTART, NULL);

	return temp;
}

int air_finish()
{
	int temp;

	temp = ioctl(dev, AIRFINISH, NULL);

	close(dev);

	return temp;
}
