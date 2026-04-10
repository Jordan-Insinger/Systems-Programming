#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>

#define devname "mycdrv"

#define CDRV_IOC_MAGIC 'Z'
#define ASP_CLEAR_BUF _IO(CDRV_IOC_MAGIC, 1)

#define E2_IOCMODE1 _IO(CDRV_IOC_MAGIC, 1)
#define E2_IOCMODE2 _IO(CDRV_IOC_MAGIC, 2)
#define MODE1 1
#define MODE2 2

void* test1_open_ioctl(void* arg) {
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

    return NULL;
}

void* test1_open(void* arg) {
    char fname[64];
    snprintf(fname, sizeof(fname), "/dev/%s", devname);

    int fd = open(fname, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "File %s was not opened.\n", fname);
        exit(1);
    }

    printf("(Thread 2): completed.\n");

    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, test1_open_ioctl, NULL);
    pthread_create(&t2, NULL, test1_open, NULL);    /* BUG FIX: was &t1 */

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}