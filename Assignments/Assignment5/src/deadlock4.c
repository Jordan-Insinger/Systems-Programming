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

/* 
 * Deadlock 4: MODE mismatch between open() and release()
 *
 * Sequence:
 *   T1: open() in MODE1 → acquires sem2, gets fd
 *   T2: ioctl(MODE2) → switches mode, up(sem2)  [sem2 now = 1 again]
 *   T3: open() → now in MODE2, succeeds without sem2
 *   T1: close(fd) → release() sees MODE2, doesn't up(sem2)
 *   T4: open() → MODE1 again (after T5 switches back)
 *        blocks on sem2 forever because no one will up it
 *
 * Relevant driver wait statements:
 *   - e2_open:    down_interruptible(&devc->sem2)   [MODE1 path]
 *   - e2_release: does NOT call up(&devc->sem2) in MODE2 path
 *   - e2_ioctl:   down_interruptible(&devc->sem2)   [MODE1 case]
 */

static int open_dev(void) {
    char fname[64];
    snprintf(fname, sizeof(fname), "/dev/%s", devname);
    int fd = open(fname, O_RDWR);
    if (fd < 0) { perror("open"); exit(1); }
    return fd;
}

void *t1_func(void *arg) {
    printf("[T1] Opening in MODE1, will hold fd...\n");
    int fd = open_dev();
    printf("[T1] Opened (fd=%d). Sleeping 4s before close.\n", fd);
    sleep(4);  
    printf("[T1] Closing fd (mode has changed to MODE2 by now).\n");
    close(fd);
    printf("[T1] Done.\n");
    return NULL;
}

void *t2_func(void *arg) {
    sleep(1);
    printf("[T2] Switching to MODE2 via ioctl.\n");
    int fd = open_dev();
    ioctl(fd, E2_IOCMODE2);
    printf("[T2] Switched to MODE2.\n");
    sleep(6);
    close(fd);
    return NULL;
}

void *t3_func(void *arg) {
    sleep(2);
    printf("[T3] Switching back to MODE1.\n");
    int fd = open_dev();
    ioctl(fd, E2_IOCMODE1);
    printf("[T3] Switched back to MODE1.\n");
    sleep(10);
    return NULL;
}

void *t4_func(void *arg) {
    sleep(3);
    printf("[T4] Attempting to open in MODE1 — should deadlock.\n");
    int fd = open_dev();
    printf("[T4] Opened (should never print).\n");
    close(fd);
    return NULL;
}

int main(void) {
    pthread_t t1, t2, t3, t4;
    pthread_create(&t1, NULL, t1_func, NULL);
    pthread_create(&t2, NULL, t2_func, NULL);
    pthread_create(&t3, NULL, t3_func, NULL);
    pthread_create(&t4, NULL, t4_func, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);
    return 0;
}
