
#include <linux/kernel.h>
#include<linux/slab.h>
#include<linux/sched/task.h>
#include<linux/module.h>
static int __init desd_init(void) {
    struct module *trav;
    printk(KERN_INFO "%s: desd_init() called.\n", THIS_MODULE->name);
    list_for_each_entry(trav, &THIS_MODULE->list, list){
        printk(KERN_INFO "%s module name :%s \n", THIS_MODULE->name, trav->name);
    }

    return 0;
}

static void __exit desd_exit(void) {
    printk(KERN_INFO "%s desd_exit() called\n", THIS_MODULE->name);

    

}

module_init(desd_init);
module_exit(desd_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module list Module");
MODULE_AUTHOR("rudra>");
