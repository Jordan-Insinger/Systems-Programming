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

#define devname "a6"

#define CDRV_IOC_MAGIC 'Z'
#define ASP_CLEAR_BUF _IO(CDRV_IOC_MAGIC, 1)

#define E2_IOCMODE1 _IO(CDRV_IOC_MAGIC, 1)
#define E2_IOCMODE2 _IO(CDRV_IOC_MAGIC, 2)
#define MODE1 1
#define MODE2 2


sem_t t1_opened;
sem_t t2_opened;
sem_t t1_released;


void* t1_func(void* arg) {
    char fname[64];
    snprintf(fname, sizeof(fname), "/dev/%s", devname);

    int fd = open(fname, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "File %s was not opened.\n", fname);
        exit(1);
    }
    
    printf("(Thread 1): opened.\n");
    sem_post(&t1_opened);

    sem_wait(&t2_opened);

    close(fd);
    printf("(Thread1): released.\n");
    sem_post(&t1_released);

    return NULL;
}

void* t2_func(void* arg) {
    char fname[64];
    snprintf(fname, sizeof(fname), "/dev/%s", devname);

    sem_wait(&t1_opened);
    int fd = open(fname, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "File %s was not opened.\n", fname);
        exit(1);
    }
    
    printf("(Thread 2): opened.\n");
    sem_post(&t2_opened);

    sem_wait(&t1_released);

    close(fd);
    printf("(Thread2): released.\n");

    return NULL;
}

int main() {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, t1_func, NULL);
    pthread_create(&t2, NULL, t2_func, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}
