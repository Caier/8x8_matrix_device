
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>
#include "../crossusb.h"

static struct usb_device* device;

static int matrix_usb_probe(struct usb_interface*, const struct usb_device_id*);
static void matrix_usb_disconnect(struct usb_interface*);
static int matrix_usb_open(struct inode* i, struct file* f);
static int matrix_usb_close(struct inode* i, struct file* f);
static ssize_t matrix_usb_write(struct file *, const char __user *, size_t, loff_t *);

const struct usb_device_id matrix_usb_table[] = {
    { USB_DEVICE(CROSS_USB_VENDOR_ID, CROSS_USB_PRODUCT_ID) },
    {}
};
MODULE_DEVICE_TABLE(usb, matrix_usb_table);

static struct usb_driver matrix_usb_driver = {
    .name = "8x8 LED Matrix USB Driver",
    .probe = matrix_usb_probe,
    .disconnect = matrix_usb_disconnect,
    .id_table = matrix_usb_table
};

static struct file_operations matrix_usb_fops = {
    .owner = THIS_MODULE,
    .write = matrix_usb_write,
    .open = matrix_usb_open,
    .release = matrix_usb_close
};

static struct usb_class_driver matrix_usb_class = {
    .name = "usb/ledmatrix%d",
    .fops = &matrix_usb_fops,
};

static int matrix_usb_probe(struct usb_interface* intf, const struct usb_device_id* id) {
    device = interface_to_usbdev(intf);

    return usb_register_dev(intf, &matrix_usb_class);
}

static void matrix_usb_disconnect(struct usb_interface* intf) {
    device = NULL;
    usb_deregister_dev(intf, &matrix_usb_class);
}

static int matrix_usb_open(struct inode* i, struct file* f) {
    return 0;
}

static int matrix_usb_close(struct inode* i, struct file* f) {
    return 0;
}

static ssize_t matrix_usb_write(struct file* file, const char __user* buf, size_t size, loff_t* off) {
    char value = 0;

    if(copy_from_user(&value, buf, min(1, size)))
        return -EFAULT;

    printk(KERN_INFO "Sending %hhx to matrix...", value);
    
    int r = usb_control_msg_send(device, 0, 1, USB_DIR_IN | USB_TYPE_VENDOR, value, 0, NULL, 0, 5000, 0);

    if(r) return r;

    return 1;
}

module_usb_driver(matrix_usb_driver);

MODULE_VERSION("0.1");
MODULE_DESCRIPTION("LED 8x8 Matrix driver");
MODULE_AUTHOR("s188741");
MODULE_LICENSE("GPL");