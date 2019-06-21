#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/jiffies.h>
#include <linux/unistd.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");

#define DEV_NAME "lamp_dev"

//light sensor
#define MAX_TIMING 12
#define LIGHT_SENSOR 8 // CS : 8
#define CLK 11         
#define MOSI 10        // Din : 10
#define MISO 9         // Dout : 9

//led
#define LED 4

//ultrasonic sensor
#define ULTRA_PIN1 23
#define ULTRA_PIN2 24

// ioctl
#define IOCTL_START_NUM 0x80
#define IOCTL_NUM1 IOCTL_START_NUM+1
#define IOCTL_NUM2 IOCTL_START_NUM+2

#define LAMP_IOCTL_NUM 'z'
#define LAMPSTART _IOWR(LAMP_IOCTL_NUM, IOCTL_NUM1, unsigned long *)
#define LAMPSTOP _IOWR(LAMP_IOCTL_NUM, IOCTL_NUM2, unsigned long *)

static dev_t dev_num;
static struct cdev *cd_cdev;

struct task_struct *light_task = NULL;
struct task_struct *ultra_task = NULL;

int light_data;
int led_check;
int ultra_check;

static void light_read(void *arg);
static void led_start(void);
static void ultra_start(void *arg);

static void light_start(void) {
	gpio_direction_output(LIGHT_SENSOR, 1); // initialized CE0
	gpio_direction_output(CLK, 1);          // initialized CLK
	udelay(1);
	gpio_direction_output(LIGHT_SENSOR, 0);
	gpio_direction_output(CLK, 0);
	gpio_direction_output(MOSI, 1); // Din Start
	udelay(10);
	gpio_direction_output(CLK, 1);
	udelay(10);
	gpio_direction_output(CLK, 0);
	gpio_direction_output(MOSI, 1); // Din DIFF
	udelay(10);
	gpio_direction_output(CLK, 1);
	udelay(10);
	gpio_direction_output(CLK, 0);
	gpio_direction_output(MOSI, 0); // Din D2
	udelay(10);
	gpio_direction_output(CLK, 1);
	udelay(10);
	gpio_direction_output(CLK, 0);
	gpio_direction_output(MOSI, 0); // Din D1
	udelay(10);
	gpio_direction_output(CLK, 1);
	udelay(10);
	gpio_direction_output(CLK, 0);
	gpio_direction_output(MOSI, 0); // Din D0
	udelay(10);
	gpio_direction_output(CLK, 1);
	udelay(10);
	gpio_direction_output(CLK, 0);
	udelay(10);
	gpio_direction_output(CLK, 1);
	udelay(10);
	// Dout start
}

static void light_read(void *arg) {
	int i;
	
	while(!kthread_should_stop()) {
		light_data = 0;

		light_start();
		gpio_direction_input(MISO);

		for(i = 0; i < MAX_TIMING; i++) {
			gpio_direction_output(CLK, 0);
			udelay(10);
		
			light_data <<= 1;
			if(gpio_get_value(MISO) == 1) {
				light_data |= 1;
			}

			gpio_direction_output(CLK, 1);
			udelay(10);
		}

		if(light_data < 1000) {
			ultra_check = 1;
		}
		else {
			ultra_check = 0;
			led_check = 0;
			led_start();
		}

		//for check
		printk("light : %d\n", light_data);
		msleep(1000);

	}
	
}

static void led_start(void) {
	if(led_check == 1) {
		gpio_set_value(LED, 1);
	}
	else if (led_check == 0) {
		gpio_set_value(LED, 0);
	}
}

static void ultra_start(void *arg) {
	ultra_check = 0;
	struct timeval start_time;
	struct timeval end_time;

	unsigned long s_time;
	unsigned long e_time;
	unsigned long distance;
	
	while(!kthread_should_stop()) {
		if(ultra_check == 0) {
			//do nothing
		}
		else if(ultra_check == 1) {
			gpio_direction_output(ULTRA_PIN1, 0);
			gpio_direction_input(ULTRA_PIN2);

			gpio_set_value(ULTRA_PIN1, 0);
			mdelay(50);

			gpio_set_value(ULTRA_PIN1, 1);
			udelay(10);

			gpio_set_value(ULTRA_PIN1, 0);

			while(gpio_get_value(ULTRA_PIN2) == 0);
			do_gettimeofday(&start_time);

			while(gpio_get_value(ULTRA_PIN2) == 1);
			do_gettimeofday(&end_time);

			s_time = (unsigned long)start_time.tv_sec*1000000 + (unsigned long)start_time.tv_usec;
			e_time = (unsigned long)end_time.tv_sec*1000000 + (unsigned long)end_time.tv_usec;

			distance = (e_time - s_time) / 58;
	
			printk("distance : %ld cm\n", distance);

			if(distance <= 20) {
				led_check = 1;
				led_start();
			
			}
			else { // distance > 20cm
				led_check = 0;
				led_start();
			}
		}
		msleep(1000);

	}
}

static long lamp_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
	switch(cmd) {
	case LAMPSTART : {
		// thread start
		light_task = kthread_run(light_read, NULL, "light_thread");	
		ultra_task = kthread_run(ultra_start, NULL, "ultra_thread");
	}
		break;

	case LAMPSTOP : {
		// thread stop
		if(light_task) {
			kthread_stop(light_task);
			printk("light task test kernel thread STOP");
		}
		if(ultra_task) {
			kthread_stop(ultra_task);
			printk("ultra task test kernel thread STOP");
		}
	}
		break;

	default :
		return -1;
	}

	return 0;
}

static int lamp_ioctl_open(struct inode *inode, struct file *file) {
	return 0;
}

static int lamp_ioctl_release(struct inode *inode, struct file *file) {
	return 0;
}

struct file_operations lamp_char_fops = {
	.unlocked_ioctl = lamp_ioctl,
	.open = lamp_ioctl_open,
	.release = lamp_ioctl_release,
};

static int __init lamp_init(void) {
	// led
	gpio_request_one(LED, GPIOF_OUT_INIT_LOW, "LED");

	// light sensor
	gpio_request(LIGHT_SENSOR, "LIGHT_SENSOR");

	// ultrasonic
	gpio_request(ULTRA_PIN1, "ULTRA_PIN1");
	gpio_request(ULTRA_PIN2, "ULTRA_PIN2");

	// chrdev
	alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
	cd_cdev = cdev_alloc();
	cdev_init(cd_cdev, &lamp_char_fops);
	cdev_add(cd_cdev, dev_num, 1);

	return 0;
}

static void __exit lamp_exit(void) {
	// light sensor
	gpio_free(LIGHT_SENSOR);
	gpio_free(CLK);
	gpio_free(MOSI);
	gpio_free(MISO);

	// led
	gpio_set_value(LED, 0);
	gpio_free(LED);

	// ultrasonic
	gpio_free(ULTRA_PIN1);
	gpio_free(ULTRA_PIN2);

	// chrdev
	cdev_del(cd_cdev);
	unregister_chrdev_region(dev_num, 1);
}

module_init(lamp_init);
module_exit(lamp_exit);
