/* Author: Jordan Insinger
*  Course: EEL5733 - ASP
*  Date: 10 Febuary, 2026
*  Assignment 2
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>

void *transformer1();
void *transformer2();
void *transformer3();

int main(int argc, char *argv[]) {
	pthread_t threads[3];
	pthread_attr_t thread_attributes;
	pthread_attr_init(&thread_attributes);
	pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_JOINABLE);


	// create thread 1 to execute transformer 1
	pthread_create(&threads[0], &thread_attributes, transformer1, NULL);

	// create thread 2 to execute transformer 2
	pthread_create(&threads[1], &thread_attributes, transformer2, NULL);

	// create thread 3 to execute transformer 3
	pthread_create(&threads[2], &thread_attributes, transformer3, NULL);

	for (int i = 0; i < 3; i++) {
		pthread_join(threads[i], NULL);
	}

	return 0;
}


void *transformer1() {
	fprintf(stdout, "thread1 created\n");
	pthread_exit(NULL);
}

void *transformer2() {
	fprintf(stdout, "thread2 created\n");
	pthread_exit(NULL);
}

void *transformer3() {
	fprintf(stdout, "thread3 created\n");
	pthread_exit(NULL);
}
