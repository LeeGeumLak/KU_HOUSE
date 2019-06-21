#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/unistd.h>
#include <linux/workqueue.h>
#include <linux/slab.h>


MODULE_LICENSE("GPL");

#define DEV_NAME "air_dev"

#define IOCTL_START_NUM 0x80
#define IOCTL_NUM1 IOCTL_START_NUM+1
#define IOCTL_NUM2 IOCTL_START_NUM+2

#define IOCTL_NUM 'z'
#define AIRSTART _IOWR(IOCTL_NUM, IOCTL_NUM1, unsigned long *)
#define AIRFINISH _IOWR(IOCTL_NUM, IOCTL_NUM2, unsigned long *)

#define MAX_TIMING 85
#define DHT11 4
#define PIN1 6
#define PIN2 13
#define PIN3 19
#define PIN4 26
#define STEPS 8
#define DELAY 1800

static int dht11_data[5] = {0, };
int check;
struct task_struct *task1 = NULL;
struct task_struct *task2 = NULL;

static dev_t dev_num;
static struct cdev *cd_cdev;

int steps[STEPS][4] = {
	{1, 0, 0, 0},
	{1, 1, 0, 0},
	{0, 1, 0, 0},
	{0, 1, 1, 0},
	{0, 0, 1, 0},
	{0, 0, 1, 1},
	{0, 0, 0, 1},
	{1, 0, 0, 1}
};

void setStep(int p1, int p2, int p3, int p4)
{
	gpio_set_value(PIN1, p1);
	gpio_set_value(PIN2, p2);
	gpio_set_value(PIN3, p3);
	gpio_set_value(PIN4, p4);
}

static int motor_play(void *arg)
{
	int i, j;

	while(!kthread_should_stop())
	{
		while(check != 0)
		{
			for(i = 0; i < 512; i++)
			{
				for(j = 0; j < STEPS; j++)
				{
					setStep(steps[j][0], steps[j][1], steps[j][2], steps[j][3]);
					udelay(900);
				}
			}
			msleep(1);
		}
		msleep(1);
	}
	return 0;
}

static int dht11_read(void *arg)
{
	while(!kthread_should_stop())
	{
		int reply = 1;

		while(reply == 1)
		{
			int last_state = 1;
			int counter = 0;
			int i = 0 , j = 0;

			dht11_data[0] = dht11_data[1] = dht11_data[2] = dht11_data[3] = dht11_data[4] = 0;

			gpio_direction_output(DHT11, 1);
			gpio_set_value(DHT11, 0);
			mdelay(18);
			gpio_set_value(DHT11, 1);
			udelay(40);
			gpio_direction_input(DHT11);

			for(i = 0; i < MAX_TIMING; i++)
			{
				counter = 0;
				while(gpio_get_value(DHT11) == last_state)
				{
					counter++;
					udelay(1);
					if(counter == 255)
					{
						break;
					}
				}

				last_state = gpio_get_value(DHT11);

				if(counter == 255)
					break;

				if((i >= 4) && (i % 2 == 0))
				{
					dht11_data[j/8] <<= 1;

					if(counter > 16)
					{
						dht11_data[j/8] |= 1;
					}

					j++;
				}
			}

			if((j >= 40) && (dht11_data[4] == ((dht11_data[0] + dht11_data[1] + dht11_data[2] + dht11_data[3]) & 0xFF)))
			{
				reply = 0;

				printk("Humidity: %d, %d Temperature = %d, %d C\n", dht11_data[0], dht11_data[1], dht11_data[2], dht11_data[3]);

				if(dht11_data[2] > 30)
				{
					if(check == 0)
					{
						printk("T > 30\n");
						check = 1;
					}
				}

				else
				{
					check = 0;
				}
			}

			else
			{
				reply = 1;
			}
		}

		msleep(10000);
	}

	return 0;
}

int th_start(void)
{
	task1 = kthread_run(dht11_read, NULL, "dht11_read");
	task2 = kthread_run(motor_play, NULL, "mothr_play");

	return 0;
}

int th_finish(void)
{
	if(task1)
	{
		kthread_stop(task1);
	}

	if(task2)
	{
		kthread_stop(task2);
	}

	return 0;
}

static long air_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch(cmd)
	{
		case AIRSTART:
			{
				th_start();

				return 0;
			}

		case AIRFINISH:
			{
				th_finish();

				return 0;
			}
	}

	return 0;
}

struct file_operations air_fops = 
{
	.unlocked_ioctl = air_ioctl,
};

static int __init simple_dht11_init(void)
{
	gpio_request(DHT11, "DHT11");

	check = 0;

	gpio_request_one(PIN1, GPIOF_OUT_INIT_LOW, "p1");
	gpio_request_one(PIN2, GPIOF_OUT_INIT_LOW, "p2");
	gpio_request_one(PIN3, GPIOF_OUT_INIT_LOW, "p3");
	gpio_request_one(PIN4, GPIOF_OUT_INIT_LOW, "p4");

//	task1 = kthread_run(dht11_read, NULL, "dht11_read");
//	task2 = kthread_run(motor_play, NULL, "motor_play");

	alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
	cd_cdev = cdev_alloc();
	cdev_init(cd_cdev, &air_fops);
	cdev_add(cd_cdev, dev_num, 1);

	return 0;
}

static void __exit simple_dht11_exit(void)
{
	cdev_del(cd_cdev);
	unregister_chrdev_region(dev_num, 1);
	
	gpio_set_value(DHT11, 0);
	gpio_free(DHT11);

	gpio_free(PIN1);
	gpio_free(PIN2);
	gpio_free(PIN3);
	gpio_free(PIN4);
}

module_init(simple_dht11_init);
module_exit(simple_dht11_exit);
