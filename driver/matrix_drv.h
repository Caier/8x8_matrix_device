#ifndef MATRIX_DRV_H 
#define MATRIX_DRV_H

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/smp.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/kconfig.h>
#include <linux/kernel_stat.h>
#include "../crossusb.h"

#define TEXT_SIZE 1024 

enum MatrixMode {
    MAT_RAW, MAT_CPU, MAT_TEXT
};

extern enum MatrixMode matrix_mode;
extern bool matrix_enabled;
extern char matrix_text[TEXT_SIZE];
extern int matrix_text_speed;
extern struct usb_device* device;
extern struct task_struct* matrix_thread;


int matrix_usb_probe(struct usb_interface*, const struct usb_device_id*);
void matrix_usb_disconnect(struct usb_interface*);
ssize_t matrix_usb_write(struct file *, const char __user *, size_t, loff_t *);
int matrix_thread_runner(void* data);
ssize_t sysfs_show(struct  device *kobj, struct device_attribute *attr, char *buf);
ssize_t sysfs_store(struct device *kobj, struct device_attribute *attr,const char *buf, size_t count);
int matrix_try_send(u8 request, u16 value, u8* data, size_t size);

#endif