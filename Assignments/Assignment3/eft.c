#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

void parse_arguments(int argc, char** argv, char* concurrency_mode, char* num_workers, char* signal_mode);
void read_input(FILE* fp);

int main(int argc, char** argv) {
	char* concurrency_mode = malloc(15 * sizeof(char));
	char* num_workers_s = malloc(5 * sizeof(char));
	char* signal_mode = malloc(7 * sizeof(char));
	int num_workers = 0;

	parse_arguments(argc, argv, concurrency_mode, num_workers_s, signal_mode);
	num_workers = atoi(num_workers_s);

	FILE *fp = fdopen(STDIN_FILENO, "r");
	read_input(fp);


	return 0;
}

void parse_arguments(int argc, char** argv, char* concurrency_mode, char* num_workers, char* signal_mode) {
	strcpy(concurrency_mode, argv[1]);
	strcpy(num_workers, argv[2]);
	strcpy(signal_mode, argv[3]);
}

void read_input(FILE* fp) {

}
