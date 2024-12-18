// user space application to test our device driver.
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include "pchar_ioctl.h"
#include<stdlib.h>

int main(int argc, char *argv[]) {
    int fd, ret;
    if(argc < 2) {
        printf("ERROR: invalid cmd line args.\n");
        return 1;
    }
    fd = open("/dev/pchar1", O_RDWR);
    if(fd < 0) {
        perror("open() failed");
        _exit(1);
    }
    if(strcmp(argv[1], "CLEAR") == 0) {
        ret = ioctl(fd, FIFO_CLEAR);
        if(ret == 0)
            printf("Device FIFO cleared.\n");
    }
    else if(strcmp(argv[1], "GETINFO") == 0) {
        devinfo_t info;
        ret = ioctl(fd, FIFO_GETINFO, &info);
        if(ret == 0)
            printf("Device FIFO info:\n\tsize = %d, len = %d, avail = %d\n", info.size, info.len, info.avail);
    }
    else if(strcmp(argv[1], "RESIZE") == 0) {
    
       long new=atoi(argv[2]);
        ret = ioctl(fd, FIFO_GETINFO, &new);
        if(ret == 0)
            printf("Device resized");
    }
    
    else
        printf("ERROR: Invalid command.\n");
    close(fd);
    return 0;
}

