#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kfifo.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include "pchar_ioctl.h"

// private device structs
typedef struct pchardev {
    struct kfifo mybuf;
    dev_t devno;
    struct cdev cdev;
    int id;
    // ...
}pchardev_t;

#define MAX 32
// device count & device data
static int DEVCNT = 4;
module_param_named(devcnt, DEVCNT, int, 0444);
static pchardev_t *devices;

// device operations
static int pchar_open(struct inode *pinode, struct file *pfile) {
    pchardev_t *dev = container_of(pinode->i_cdev, pchardev_t, cdev);
    pfile->private_data = dev;
    pr_info("%s: pchar_open() called for pchar%d.\n", THIS_MODULE->name, dev->id);
    return 0;
}

static int pchar_close(struct inode *pinode, struct file *pfile) {
    pchardev_t *dev = (pchardev_t *)pfile->private_data;
    pr_info("%s: pchar_close() called for pchar%d.\n", THIS_MODULE->name, dev->id);
    return 0;
}

static ssize_t pchar_write(struct file *pfile, const char * __user ubuf, size_t bufsize, loff_t *pf_pos) {
    pchardev_t *dev = (pchardev_t *)pfile->private_data;
    int ret, nbytes;
    pr_info("%s: pchar_write() called for pchar%d.\n", THIS_MODULE->name, dev->id);
    ret = kfifo_from_user(&dev->mybuf, ubuf, bufsize, &nbytes);
    if(ret < 0) {
        pr_err("%s: kfifo_from_user() failed for pchar%d.\n", THIS_MODULE->name, dev->id);
        return ret;
    }
    pr_info("%s: pchar_write() written %d bytes in pchar%d.\n", THIS_MODULE->name, nbytes, dev->id);
    return nbytes;
}

static ssize_t pchar_read(struct file *pfile, char * __user ubuf, size_t bufsize, loff_t *pf_pos) {
    pchardev_t *dev = (pchardev_t *)pfile->private_data;
    int ret, nbytes;
    pr_info("%s: pchar_read() called for pchar%d.\n", THIS_MODULE->name, dev->id);
    ret = kfifo_to_user(&dev->mybuf, ubuf, bufsize, &nbytes);
    if(ret < 0) {
        pr_err("%s: kfifo_to_user() failed for pchar%d.\n", THIS_MODULE->name, dev->id);
        return ret;
    }
    pr_info("%s: pchar_read() read %d bytes in pchar%d.\n", THIS_MODULE->name, nbytes, dev->id);
    return nbytes;
}
static long pchar_ioctl(struct file *pfile, unsigned int cmd, unsigned long param) {
     pchardev_t *dev = (pchardev_t *)pfile->private_data;
    devinfo_t info;
    int ret;
    switch (cmd)
    {
    case FIFO_CLEAR:
        kfifo_reset(&dev->mybuf);
        printk(KERN_INFO "%s: pchar_ioctl() dev buffer is cleared.\n", THIS_MODULE->name);
        return 0;
    case FIFO_GETINFO:
        info.size = kfifo_size(&dev->mybuf);
        info.len = kfifo_len(&dev->mybuf);
        info.avail = kfifo_avail(&dev->mybuf);
        ret = copy_to_user((void*)param, &info, sizeof(info));
        if(ret < 0) {
            printk(KERN_ERR "%s: copy_to_user() failed in pchar_ioctl().\n", THIS_MODULE->name);
            return ret;
        }
        printk(KERN_INFO "%s: pchar_ioctl() read dev buffer info.\n", THIS_MODULE->name);
        return 0;
        
        case FIFO_RESIZE:
            int ret;
            // allocate temp array to store fifo contents - kmalloc() (of length same as fifo).
            char * buf=kmalloc(MAX, GFP_KERNEL);
            if(IS_ERR(buf)){
                pr_err("%s kmalloc has failed\n", THIS_MODULE->name);
                return 1;
            }
            // copy fifo contents into that temp array - kfifo_out()
            int nbytes;
            nbytes=kfifo_out(&dev->mybuf, buf, MAX);
            pr_info("%s released %d bytes from kfifo\n", THIS_MODULE->name, nbytes);
            // release kfifo memory - kfifo_free()
            kfifo_free(&dev->mybuf);
           
            // allocate new memory for the fifo of size "param" (3rd arg) - kfifo_alloc()
             ret= kfifo_alloc(&dev->mybuf, param, GFP_KERNEL);
              if(ret!=0){
                 pr_err("%s kfifo_free has failed\n", THIS_MODULE->name);
                return 1;
            }
            // copy contents from temp memory into kfifo - kfifo_in()
                kfifo_in(&dev->mybuf,buf, nbytes);
            // release temp array - kfree()
           kfree(buf);
           
            return 0;
        
    }
    printk(KERN_ERR "%s: invalid command in pchar_ioctl().\n", THIS_MODULE->name);    
    return -EINVAL; // invalid command
}
static struct file_operations pchar_fops = {
    .owner = THIS_MODULE, 
    .open = pchar_open,
    .release = pchar_close,
    .read = pchar_read,
    .write = pchar_write,
    .unlocked_ioctl = pchar_ioctl
};

// other global vars
static dev_t devno;
static int major;
static struct class *pclass;

static int __init pchar_init(void) {
    int ret, i;
    struct device *pdevice;
    dev_t devnum;
    pr_info("%s: pchar_init() called.\n", THIS_MODULE->name);
    // allocate array device private structs
    devices = kmalloc(DEVCNT * sizeof(pchardev_t), GFP_KERNEL);
    if(IS_ERR(devices)) {
        pr_err("%s: kmalloc() failed.\n", THIS_MODULE->name);
        ret = -1;
        goto kmalloc_failed;
    }
    // allocate device numbers
    ret = alloc_chrdev_region(&devno, 0, DEVCNT, "pchar");
    if(ret != 0) {
        pr_err("%s: alloc_chrdev_region() failed.\n", THIS_MODULE->name);
        goto alloc_chrdev_region_failed;
    }
    major = MAJOR(devno);
    pr_info("%s: alloc_chrdev_region() allocated device number: major = %d\n", THIS_MODULE->name, major);
    // create device class
    pclass = class_create("pchar_class");
    if(IS_ERR(pclass)) {
        pr_err("%s: class_create() failed for pchar%d.\n", THIS_MODULE->name, i);
        ret = -1;
        goto class_create_failed;
    }
    pr_info("%s: class_create() created device class\n", THIS_MODULE->name);
    // create device files
    for(i=0; i<DEVCNT; i++) {
        devnum = MKDEV(major, i);
        pdevice = device_create(pclass, NULL, devnum, NULL, "pchar%d", i);
        if(IS_ERR(pdevice)) {
            pr_err("%s: device_create() failed for pchar%d.\n", THIS_MODULE->name, i);
            ret = -1;
            goto device_create_failed;
        }
        pr_info("%s: device_create() created device file pchar%d\n", THIS_MODULE->name, i);
    }
    // initialize and add cdevs into kernel
    for(i=0; i<DEVCNT; i++) {
        devnum = MKDEV(major, i);
        devices[i].cdev.owner = THIS_MODULE;
        cdev_init(&devices[i].cdev, &pchar_fops);
        ret = cdev_add(&devices[i].cdev, devnum, 1);
        if(ret != 0) {
            pr_err("%s: cdev_add() failed for pchar%d.\n", THIS_MODULE->name, i);
            goto cdev_add_failed;
        }
        pr_info("%s: cdev_add() added cdev into kernel for pchar%d\n", THIS_MODULE->name, i);
    }
    // initialize device info
    for(i=0; i<DEVCNT; i++) {
        devices[i].id = i;
        devices[i].devno = MKDEV(major, i);
        ret = kfifo_alloc(&devices[i].mybuf, MAX, GFP_KERNEL);
        if(ret != 0) {
            pr_err("%s: kfifo_alloc() failed for pchar%d.\n", THIS_MODULE->name, i);
            goto kfifo_alloc_failed;
        }
        pr_info("%s: kfifo_alloc() allocated fifo for pchar%d\n", THIS_MODULE->name, i);
    }
    // all initialization successful
    return 0;

kfifo_alloc_failed:
    for(i = i - 1; i >= 0; i--) {
        kfifo_free(&devices[i].mybuf);
    }
    i = DEVCNT;
cdev_add_failed:
    for(i = i - 1; i >= 0; i--) {
        cdev_del(&devices[i].cdev);
    }
    i = DEVCNT;
device_create_failed:
    for(i = i - 1; i >= 0; i--) {
        devnum = MKDEV(major, i);
        device_destroy(pclass, devnum);
    }
    class_destroy(pclass);
class_create_failed:
    unregister_chrdev_region(devno, DEVCNT);
alloc_chrdev_region_failed:
    kfree(devices);
kmalloc_failed:
    return ret;
}

static void __exit pchar_exit(void) {
    int i;
    pr_info("%s: pchar_exit() called.\n", THIS_MODULE->name);
    // deinitialize device info
    for(i=0; i<DEVCNT; i++) {
        kfifo_free(&devices[i].mybuf);
        pr_info("%s: kfifo_free() released fifo for pchar%d\n", THIS_MODULE->name, i);
    }
    // delete cdev from kernel
    for(i=0; i<DEVCNT; i++) {
        cdev_del(&devices[i].cdev);
        pr_info("%s: cdev_del() removed cdev from kernel for pchar%d\n", THIS_MODULE->name, i);
    }
    // destroy device files
    for(i=0; i<DEVCNT; i++) {
        device_destroy(pclass, devices[i].devno);
        pr_info("%s: device_destroy() destroyed device file pchar%d\n", THIS_MODULE->name, i);
    }
    // destroy device class
    class_destroy(pclass);
    pr_info("%s: class_destroy() destroyed device class\n", THIS_MODULE->name);
    // unregister device numbers
    unregister_chrdev_region(devno, DEVCNT);
    pr_info("%s: unregister_chrdev_region() released device numbers: major = %d\n", THIS_MODULE->name, major);
}

module_init(pchar_init);
module_exit(pchar_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Multiple devices - Pseduo Char Driver Demo");
MODULE_AUTHOR("Unknown <anonymous@gmail.com>");