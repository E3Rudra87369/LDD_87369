#include <linux/module.h>
#include <linux/kernel.h>
#include<linux/kthread.h>
#include<linux/delay.h>
#include<asm/delay.h>

int thread_fxn(void *data){
    int i; 
    for(i=1;i<=10;i++){
        printk(KERN_INFO "%s thread_fxn() kthread(%d) count=%d", THIS_MODULE->name,current->pid,i);
        msleep(1000);
    }
    return 0;
}

static int __init desd_init(void) {
	struct task_struct *task;
    task=kthread_run(thread_fxn, NULL, "fxnthread");

    printk(KERN_INFO "%s: desd_init() called new thread created %d\n", THIS_MODULE->name, task->pid);
    
    return 0;
}

static void __exit desd_exit(void) {
    printk(KERN_INFO "%s: desd_exit() called.\n", THIS_MODULE->name);
    
}

module_init(desd_init);
module_exit(desd_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Hello Kernel Module");
MODULE_AUTHOR("rudra>");
    
