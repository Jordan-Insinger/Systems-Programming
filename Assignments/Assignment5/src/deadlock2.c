#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#define devname "a6"

#define CDRV_IOC_MAGIC 'Z'
#define ASP_CLEAR_BUF _IO(CDRV_IOC_MAGIC, 1)

#define E2_IOCMODE1 _IO(CDRV_IOC_MAGIC, 1)
#define E2_IOCMODE2 _IO(CDRV_IOC_MAGIC, 2)
#define MODE1 1
#define MODE2 2

void* _ioctl_2(void* arg) {

    int fd = *(int*)arg;
    
    long ret = ioctl(fd, E2_IOCMODE2);
    printf("(Thread 1): ioctl(MODE2) returned %ld\n", ret);
    printf("(Thread 1): completed.\n");
    return NULL;
}

void* _ioctl_1(void* arg) {

    int fd = *(int*)arg;
    
    long ret = ioctl(fd, E2_IOCMODE1);
    printf("(Thread 1): ioctl(MODE1) returned %ld\n", ret);
    printf("(Thread 1): completed.\n");
    return NULL;
}

void* t1_open_ioctl(void* arg) {
    char fname[64];
    snprintf(fname, sizeof(fname), "/dev/%s", devname);

    int fd = open(fname, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "File %s was not opened.\n", fname);
        exit(1);
    }

    long ret = ioctl(fd, E2_IOCMODE2);
    printf("(Thread 1): ioctl(MODE2) returned %ld\n", ret);
    printf("(Thread 1): completed.\n");

    sleep(3);

    printf("(Thread 1): Attempting to switch back to MODE1 via ioctl.\n");
    long ret = ioctl(fd, E2_IOCMODE1);
    printf("(Thread 1): ioctl(MODE1) returned %ld\n", ret);
    printf("(Thread 1): completed.\n");

    return NULL;
}

void* t2_open_ioctl(void* arg) {
    char fname[64];
    snprintf(fname, sizeof(fname), "/dev/%s", devname);

    int fd = open(fname, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "File %s was not opened.\n", fname);
        exit(1);
    }

    printf("(Thread 2): calling ioctl(MODE1)\n");
    long ret = ioctl(fd, E2_IOCMODE1);
    printf("(Thread 2): ioctl(MODE1) returned %ld\n", ret);
    printf("(Thread 2): completed.\n");
    return NULL;
}

int main() {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, t1_func, NULL);
    sleep(1);
    pthread_create(&t2, NULL, t2_func, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}
