#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

#define DEV_NAME "counter_dev"

#define IOCTL_START_NUM 0x80
#define IOCTL_NUM1 IOCTL_START_NUM+1
#define IOCTL_NUM2 IOCTL_START_NUM+2

#define IOCTL_NUM 'z'
#define COUNTERSTART _IOWR(IOCTL_NUM, IOCTL_NUM1, unsigned long *)
#define COUNTERFINISH _IOWR(IOCTL_NUM, IOCTL_NUM2, unsigned long *)

#define SENSOR1 17

#define ultraPIN1 19
#define ultraPIN2 26

#define ultraPIN3 22
#define ultraPIN4 23


int person_check;
int ultra1_check;
int ultra2_check;
int pir_check=0;
size_t able;

unsigned long ultra1time;
unsigned long pirtime;
unsigned long ultra2time;


int pircount;
int ultra1count = 0;
int ultra2count = 0;
int ret;
int ultranum =0;
char *temp;
char *people;
struct task_struct *ultra1_task = NULL;
struct task_struct *ultra2_task = NULL;

static int irq_num;
static dev_t dev_num;
static struct cdev *cd_cdev;

static int ultra1_start(void *arg)
{

	ultra1time = jiffies;
	struct timeval start_time;
	struct timeval end_time;

	unsigned long s_time;
	unsigned long e_time;
	unsigned long distance;

	while(!kthread_should_stop()) {
		gpio_direction_output(ultraPIN1, 0);
		
		gpio_direction_input(ultraPIN2);

		gpio_set_value(ultraPIN1, 0);
		mdelay(50);

		gpio_set_value(ultraPIN1, 1);
		udelay(10);

		gpio_set_value(ultraPIN1, 0);

		while(gpio_get_value(ultraPIN2) == 0);
		do_gettimeofday(&start_time);

		while(gpio_get_value(ultraPIN2) == 1);
		do_gettimeofday(&end_time);

		s_time = (unsigned long)start_time.tv_sec*1000000 + (unsigned long)start_time.tv_usec;
		e_time = (unsigned long)end_time.tv_sec*1000000 + (unsigned long)end_time.tv_usec;

		distance = (e_time - s_time) / 58;

		if(distance <= 20)
		{
			ultra1count++;
			ultra1time = jiffies;
			printk("ult1 : %d\n", ultra1count);
			msleep(1000);
		}
		else if(distance > 100)
		{

			unsigned long a = jiffies- ultra1time;
			if(a >600)
			{
				ultra1count =0;
			}
		}

		msleep(1000);

	}

	return 0;

}
static int ultra2_start(void *arg)
{
	ultra2_check=0;
	ultra2time = jiffies;
	struct timeval start_time;
	struct timeval end_time;

	unsigned long s_time;
	unsigned long e_time;
	unsigned long distance;

	while(!kthread_should_stop()) {
		gpio_direction_output(ultraPIN3, 0);
		gpio_direction_input(ultraPIN4);

		gpio_set_value(ultraPIN3, 0);
		mdelay(50);

		gpio_set_value(ultraPIN3, 1);
		udelay(10);

		gpio_set_value(ultraPIN3, 0);

		while(gpio_get_value(ultraPIN4) == 0);
		do_gettimeofday(&start_time);

		while(gpio_get_value(ultraPIN4) == 1);
		do_gettimeofday(&end_time);

		s_time = (unsigned long)start_time.tv_sec*1000000 + (unsigned long)start_time.tv_usec;
		e_time = (unsigned long)end_time.tv_sec*1000000 + (unsigned long)end_time.tv_usec;

		distance = (e_time - s_time) / 58;

		if(distance <= 20)
		{
			ultra2count++;
			printk("ult2 : %d\n", ultra2count);
			ultra2time = jiffies;
			msleep(1000);
		}
		else if(distance > 100)
		{

			unsigned long a = jiffies- ultra2time;
			if(a >600)
			{
				ultra2count =0;
			}
		}
		msleep(1000);

	}

	return 0;

}

static irqreturn_t simple_sensor_isr(int irq, void* dev_id)
{
	pirtime= jiffies;

	if(ultra1count != 0 || ultra2count != 0)
	{
		pircount += ultra2count;
		ultra2count = 0;
		pircount -= ultra1count;
		ultra1count = 0;

		if(pircount < 0)
		{
			pircount = 0;
		}
	
		temp = (char *)kmalloc(sizeof(char *), GFP_KERNEL);
		sprintf(temp, "%d", pircount);
		copy_to_user(people, temp, sizeof(temp));		
	}

	return IRQ_HANDLED;

}

int th_start(void)
{
	ultra1_task = kthread_run(ultra1_start, NULL, "ultra1_start");
	ultra2_task = kthread_run(ultra2_start, NULL, "ultra2_start");

	return 0;
}

int th_finish(void)
{
	if(ultra1_task)
	{
		kthread_stop(ultra1_task);
	}

	if(ultra2_task)
	{
		kthread_stop(ultra2_task);
	}

	return 0;
}

static long counter_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch(cmd)
	{
		case COUNTERSTART:
			{
				people = (char *)kmalloc(sizeof(char *), GFP_KERNEL);
				people = (char *)arg;
				th_start();

				return 0;
			}

		case COUNTERFINISH:
			{
				th_finish();

				return 0;
			}
	}

	return 0;
}

struct file_operations counter_fops =
{
	.unlocked_ioctl = counter_ioctl,
};

static int __init simple_sensor_init(void)
{

	int ret;
	pircount = 0;
	gpio_request(ultraPIN1, "ultraPIN1");
	gpio_request(ultraPIN2, "ultraPIN2");
	gpio_request(ultraPIN3, "ultraPIN3");
	gpio_request(ultraPIN4, "ultraPIN4");


	alloc_chrdev_region(&dev_num,0,1,DEV_NAME);
	cd_cdev = cdev_alloc();
	cdev_init(cd_cdev, &counter_fops);
	cdev_add(cd_cdev, dev_num, 1);

	gpio_request_one(SENSOR1, GPIOF_IN, "sensor1");
	irq_num = gpio_to_irq(SENSOR1);
	ret = request_irq(irq_num, simple_sensor_isr, IRQF_TRIGGER_RISING, "sensor_irq", NULL);
	pirtime = jiffies;
	if(ret)
	{
		printk(KERN_ERR "unable to request IRQ: %d\n", ret);
		free_irq(irq_num, NULL);
	}


	return 0;
}
static void __exit simple_sensor_exit(void)
{
	cdev_del(cd_cdev);
	unregister_chrdev_region(dev_num, 1);
	gpio_free(ultraPIN1);
	gpio_free(ultraPIN2);
	gpio_free(ultraPIN3);
	gpio_free(ultraPIN4);

	free_irq(irq_num, NULL);
	gpio_free(SENSOR1);

}
module_init(simple_sensor_init);
module_exit(simple_sensor_exit);
