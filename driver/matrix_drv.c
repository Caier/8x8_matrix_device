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
#include "matrix_drv.h"

enum MatrixMode matrix_mode = MAT_TEXT;
bool matrix_enabled = true;
char matrix_text[TEXT_SIZE] = "hello world";
int matrix_text_speed = 200;
struct usb_device* device;
struct task_struct* matrix_thread = NULL;

DEFINE_MUTEX(matrix_lock);

struct class* matrix_class = NULL;
struct device* matrix_device = NULL;
DEVICE_ATTR(mode, 0660, sysfs_show, sysfs_store);
DEVICE_ATTR(enabled, 0660, sysfs_show, sysfs_store);
DEVICE_ATTR(text, 0660, sysfs_show, sysfs_store);
DEVICE_ATTR(text_speed, 0660, sysfs_show, sysfs_store);

const struct usb_device_id matrix_usb_table[] = {
    { USB_DEVICE(CROSS_USB_VENDOR_ID, CROSS_USB_PRODUCT_ID) },
    {}
};
MODULE_DEVICE_TABLE(usb, matrix_usb_table);

struct usb_driver matrix_usb_driver = {
    .name = "8x8 LED Matrix USB Driver",
    .probe = matrix_usb_probe,
    .disconnect = matrix_usb_disconnect,
    .id_table = matrix_usb_table
};

int matrix_open_release(struct inode* i, struct file* f) {
    return 0;
}

struct file_operations matrix_usb_fops = {
    .owner = THIS_MODULE,
    .write = matrix_usb_write,
    .open = matrix_open_release,
    .release = matrix_open_release
};

struct usb_class_driver matrix_usb_class = {
    .name = "usb/ledmatrix%d",
    .fops = &matrix_usb_fops,
};

int matrix_try_send(u8 request, u16 value, u8* data, size_t size) {
    mutex_lock(&matrix_lock);
    int r;
    for(int i = 0; i < 10; i++) {
        r = usb_control_msg(device, usb_sndctrlpipe(device, 0), request, USB_DIR_OUT | USB_TYPE_VENDOR, value, 0, data, size, 50);
        if(r >= 0)
            break;
    }
    mutex_unlock(&matrix_lock);

    return r;
}
EXPORT_SYMBOL(matrix_try_send);


ssize_t sysfs_show(struct device *kobj, struct device_attribute *attr, char *buf) {
    if(attr == &dev_attr_mode) {
        if(matrix_mode == MAT_RAW)
            return sprintf(buf, "[RAW] CPU TEXT\n");
        else if(matrix_mode == MAT_CPU)
            return sprintf(buf, "RAW [CPU] TEXT\n");
        else if(matrix_mode == MAT_TEXT)
            return sprintf(buf, "RAW CPU [TEXT]\n");
    } else if(attr == &dev_attr_enabled) {
        return sprintf(buf, "%hhd\n", matrix_enabled);
    } else if(attr == &dev_attr_text) {
        return sprintf(buf, "%s\n", matrix_text);
    } else if(attr == &dev_attr_text_speed) {
        return sprintf(buf, "%d\n", matrix_text_speed);
    }

    return 0;
}

ssize_t sysfs_store(struct device *kobj, struct device_attribute *attr, const char *buf, size_t count) {
    if(attr == &dev_attr_mode) {
        char inp[30];
        sscanf(buf, "%29s", inp);
        if(strcmp(inp, "RAW") == 0)
            matrix_mode = MAT_RAW;
        else if(strcmp(inp, "CPU") == 0)
            matrix_mode = MAT_CPU;
        else if(strcmp(inp, "TEXT") == 0)
            matrix_mode = MAT_TEXT;
        else return -EINVAL;
        return count;
    } else if(attr == &dev_attr_enabled) {
        int i;
        int r = sscanf(buf, "%d", &i);
        if(r != 1)
            return -EINVAL;
        matrix_enabled = i != 0;
        r = matrix_try_send(MATRIX_ENABLE_RQ, matrix_enabled, NULL, 0);
        if(r < 0) return r;
        return count; 
    } else if(attr == &dev_attr_text) {
        memset(matrix_text, 0, TEXT_SIZE);
        strncpy(matrix_text, buf, min(count + 1, TEXT_SIZE - 1));
        return count;
    } else if(attr == &dev_attr_text_speed) {
        int i;
        int r = sscanf(buf, "%d", &i);
        if(r != 1 || i > 5000 || i < 30)
            return -EINVAL;
        matrix_text_speed = i;
        return count;
    }

    return 0;
}

int matrix_usb_probe(struct usb_interface* intf, const struct usb_device_id* id) {
    mutex_lock(&matrix_lock);
    device = interface_to_usbdev(intf);
    
    int r = usb_register_dev(intf, &matrix_usb_class);
    if(r < 0) goto ret;

    matrix_class = class_create("ledmatrix");
    if(IS_ERR(matrix_class)) {
        printk(KERN_ERR "failed to create ledmatrix class");
        goto ret;
    }

    matrix_device = device_create(matrix_class, NULL, intf->dev.devt, NULL, "ledmatrix%d", intf->minor);
    if(IS_ERR(matrix_device)) {
        printk(KERN_ERR "failed to create device");
        goto ret;
    }

    r = device_create_file(matrix_device, &dev_attr_mode);
    r = device_create_file(matrix_device, &dev_attr_enabled);
    r = device_create_file(matrix_device, &dev_attr_text);
    r = device_create_file(matrix_device, &dev_attr_text_speed);
    if(r < 0) {
        printk(KERN_WARNING "failed to create at least one ledmatrix sysfs attribute");
    }

    matrix_thread = kthread_run(matrix_thread_runner, NULL, "matrix_thread");

    ret:
    mutex_unlock(&matrix_lock);
    return 0;
}

void matrix_usb_disconnect(struct usb_interface* intf) {
    if(matrix_thread)
        kthread_stop(matrix_thread);

    mutex_lock(&matrix_lock);
    device_remove_file(matrix_device, &dev_attr_mode);
    device_remove_file(matrix_device, &dev_attr_enabled);
    device_remove_file(matrix_device, &dev_attr_text);
    device_remove_file(matrix_device, &dev_attr_text_speed);
    device_destroy(matrix_class, matrix_device->devt);
    class_destroy(matrix_class);
    device = NULL;
    mutex_unlock(&matrix_lock);

    usb_deregister_dev(intf, &matrix_usb_class);
}

ssize_t matrix_usb_write(struct file* file, const char __user* buf, size_t size, loff_t* off) {
    char* pay = (char*)kmalloc(8, GFP_KERNEL);

    if(copy_from_user(pay, buf, min(8, size)))
        return -EFAULT;

    int r = matrix_try_send(1, 0, pay, 8);
    kfree(pay);

    if(r) return r;
    return min(8, size);
}

module_usb_driver(matrix_usb_driver);

MODULE_VERSION("0.1");
MODULE_DESCRIPTION("LED 8x8 Matrix driver");
MODULE_AUTHOR("s188741");
MODULE_LICENSE("GPL");