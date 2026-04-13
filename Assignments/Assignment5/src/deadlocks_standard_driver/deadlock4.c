#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define devname "a6"
#define CDRV_IOC_MAGIC 'Z'
#define E2_IOCMODE1 _IO(CDRV_IOC_MAGIC, 1)
#define E2_IOCMODE2 _IO(CDRV_IOC_MAGIC, 2)

static int open_dev(void) {
    char fname[64];
    snprintf(fname, sizeof(fname), "/dev/%s", devname);
    int fd = open(fname, O_RDWR);
    if (fd < 0) { perror("open"); exit(1); }
    return fd;
}

void *t1_func(void *arg) {
    printf("(Thread 1) Opening in MODE1.\n");
    int fd = open_dev();
    ioctl(fd, E2_IOCMODE2);
    sleep(999);

    printf("[T1] Done.\n");
    return NULL;
}

void *t2_func(void *arg) {
    int fd = open_dev();
    printf("(Thread 2) device opened.\n");
    ioctl(fd, E2_IOCMODE1);
    printf("Thread 2 exiting.\n");
    return NULL;
}


int main(void) {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, t1_func, NULL);
    sleep(2);
    pthread_create(&t2, NULL, t2_func, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}
