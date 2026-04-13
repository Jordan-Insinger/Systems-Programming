#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <signal.h>

#define devname "a6"

#define CDRV_IOC_MAGIC 'Z'
#define ASP_CLEAR_BUF _IO(CDRV_IOC_MAGIC, 1)

#define E2_IOCMODE1 _IO(CDRV_IOC_MAGIC, 1)
#define E2_IOCMODE2 _IO(CDRV_IOC_MAGIC, 2)
#define MODE1 1
#define MODE2 2


sem_t t2_opened;
sem_t t2_mode2switch;


void* t1_func(void* arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    char fname[64];
    snprintf(fname, sizeof(fname), "/dev/%s", devname);

    int fd = open(fname, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "File %s was not opened.\n", fname);
        exit(1);
    }
    
    printf("(Thread 1): opened.\n");
    return NULL;
}

void* t2_func(void* arg) {
    printf("(Thread 2): entered thread 2 func.\n");
    char fname[64];
    snprintf(fname, sizeof(fname), "/dev/%s", devname);

    int fd = open(fname, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "File %s was not opened.\n", fname);
        exit(1);
    }
    
    printf("(Thread 2): opened.\n");
    sem_post(&t2_opened);

    ioctl(fd, E2_IOCMODE2);
    sem_post(&t2_mode2switch);
    ioctl(fd, E2_IOCMODE1);

    return NULL;
}

void* t3_func(void* arg) {
    printf("(Thread 3): entered thread 3 func.\n");
    char fname[64];
    snprintf(fname, sizeof(fname), "/dev/%s", devname);

    sem_wait(&t2_opened);
    int fd = open(fname, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "File %s was not opened.\n", fname);
        exit(1);
    }
    
    printf("(Thread 3): opened.\n");

    sem_wait(&t2_mode2switch);
    sleep(2);
    char buf[256];
    read(fd, buf, sizeof(buf));
    printf("(Thread3): finished read.\n");

    return NULL;
}

int main() {
    pthread_t t1, t2, t3;

    pthread_create(&t1, NULL, t1_func, NULL);
    sleep(2);
    printf("Killing Thread 1\n");
    pthread_cancel(t1);
    pthread_create(&t2, NULL, t2_func, NULL);
    pthread_create(&t3, NULL, t3_func, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    return 0;
}
