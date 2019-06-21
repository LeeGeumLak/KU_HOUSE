#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <uapi/linux/string.h>
#include <linux/kmod.h>
#include <linux/kthread.h>

#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/unistd.h>
#include <linux/cdev.h>

#define DEV_NAME "fridge_dev"

MODULE_LICENSE("GPL");

#define TRIG_PIN 23
#define ECHO_PIN 24
#define LED_PIN 13

#define IOCTL_START_NUM 0x80
#define IOCTL_NUM1 IOCTL_START_NUM+1
#define IOCTL_NUM2 IOCTL_START_NUM+2


#define IOCTL_NUM 'z'
#define FRIDGESTART _IOWR(IOCTL_NUM, IOCTL_NUM1, unsigned long *)
#define FRIDGEFINISH _IOWR(IOCTL_NUM, IOCTL_NUM2, unsigned long *)


struct task_struct *ultra_task = NULL;
struct task_struct *camera_task = NULL;

int camera_check;
int led_check;

static dev_t dev_num;
static struct cdev *cd_cdev;

static void led_start(void);
static int  camera_start(void *arg) ;
static int  ultra_start(void *arg);


static void led_start(void){
	if(led_check==1){
		gpio_set_value(LED_PIN,1);
	}
	else if(led_check == 0) {
		gpio_set_value(LED_PIN,0);
	}
}

static int  camera_start(void *arg){
	char *envp[] = { "HOME=/", NULL };
	char *argv[] = { "/home/pi/capture.sh", NULL};
	
	while(!kthread_should_stop()){
		if(camera_check == 0){
			led_check = 0;
			led_start();
		}
		else if(camera_check == 1){
			led_check = 1;
			led_start();
			call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);	
		}
		msleep(1000);
	}
	return 0;
}

static int  ultra_start(void *arg){
	struct timeval start_time;
	struct timeval end_time;
	unsigned long stime;
	unsigned long etime;
	unsigned long distance;
	int check;
	check = 0;

	while(!kthread_should_stop()){
	
		gpio_direction_output(TRIG_PIN, 0);
		gpio_direction_input(ECHO_PIN);

		gpio_set_value(TRIG_PIN, 0);
		mdelay(50);
		
		gpio_set_value(TRIG_PIN, 1);
		udelay(10);

		gpio_set_value(TRIG_PIN, 0);

		while(gpio_get_value(ECHO_PIN)==0);
		do_gettimeofday(&start_time);

		while(gpio_get_value(ECHO_PIN)==1);
		do_gettimeofday(&end_time);
	
		stime = (unsigned long)start_time.tv_sec*1000000 + (unsigned long)start_time.tv_usec;

		etime = (unsigned long)end_time.tv_sec*1000000 + (unsigned long)end_time.tv_usec;

		distance = (etime-stime) /58;
		
		if(distance < 40){
			printk("camera start distance : %ld \n", distance);
			camera_check = 1;

		}
		else{
			printk("distance : %ld cm \n", distance);
			camera_check = 0;
		}
		msleep(1000);
	}
	return 0;
}


int th_start(void){
	ultra_task = kthread_run(ultra_start, NULL, "ultra_thread");
	camera_task = kthread_run(camera_start, NULL, "camera_thread");
}

int th_finish(void){
	if(camera_task){
		kthread_stop(camera_task);
		printk("cameara task test kenel thread STOP");
	}
	if(ultra_task){
		kthread_stop(ultra_task);
		printk("ultra task test kernel thread STOP");
	}
}

static long fridge_ioctl(struct file *file, unsigned int cmd, unsigned long arg){

	switch(cmd){
		case FRIDGESTART:{
			th_start();
			return 0;
		}
		case FRIDGEFINISH:{
			th_finish();
			return 0;
		}
	}
	return 0;
}

struct file_operations fridge_fops = {
	.unlocked_ioctl = fridge_ioctl,
};

				
static int __init fridge_init(void){
	
	gpio_request_one(LED_PIN, GPIOF_OUT_INIT_LOW,"LED");

	gpio_request(TRIG_PIN, "TRIG_PIN");
	gpio_request(ECHO_PIN, "ECHO_PIN");
	
	alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
	cd_cdev = cdev_alloc();
	cdev_init(cd_cdev, &fridge_fops);		
	cdev_add(cd_cdev, dev_num, 1);

	return 0;
}

static void __exit fridge_exit(void){
	
	gpio_set_value(LED_PIN,0);
	gpio_free(LED_PIN);

	gpio_free(TRIG_PIN);
	gpio_free(ECHO_PIN);
	
	cdev_del(cd_cdev);
	unregister_chrdev_region(dev_num,1);
}


module_init(fridge_init);
module_exit(fridge_exit);
