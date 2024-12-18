#ifndef __PCHAR_IOCTL_H
#define __PCHAR_IOCTL_H

#include <linux/ioctl.h>

typedef struct devinfo {
    short size; // capacity
    short len; // used bytes
    short avail; // free bytes
}devinfo_t;

#define FIFO_CLEAR      _IO('x', 1)
#define FIFO_GETINFO    _IOR('x', 2, devinfo_t)
#define   FIFO_RESIZE   _IOW('x', 3, long)

#endif

