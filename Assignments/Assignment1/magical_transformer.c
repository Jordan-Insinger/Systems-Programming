/* Author: Jordan Insinger
 * Course: EEL5733 - ASP
 * Date: 30 January, 2026
 * Assignment 1
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

void parse_arguments(int argc, char *argv[], char functionality[][32], char stream[][12]);

int main(int argc, char *argv[]) {

	char functionality[argc-1][32];
	char stream[argc-1][12];

	if (argc == 1) {
		fprintf(stdout, "No output requested, have a nice day!\n");
		return 0;
	}
	
	// parse arguments
	parse_arguments(argc, argv, functionality, stream); // pass arguments for parsing

	// save original stdout and stderr fds
	int saved_stdout = dup(STDOUT_FILENO);
	int saved_stderr = dup(STDERR_FILENO);

	// create pipe
	int pipefd1[2]; // pipe for stdout
	pipe(pipefd1);

	int pipefd2[2]; // pipe for stderr
	pipe(pipefd2);
	
	// Fork and exec Transformer1 if there are args
	pid_t pid = fork();

	if (pid == 0) { // child process - execute Transformer 1
		dup2(pipefd1[1], STDOUT_FILENO); // redirect stdout to pipe1 
		dup2(pipefd2[1], STDERR_FILENO); // redirect stderr to pipe2 

		char *args[] = {"./Transformer1", NULL};
		execv("./Transformer1", args);	
	}
	else if (pid != -1) { // parent process - wait and break
		int status;
		waitpid(pid, &status, 0);

		if (!WIFEXITED(status)) { // false if child process error
			fprintf(stdout, "error executing child process: Transformer 1.\n");
			return -1;	
		}

		// flush streams and close write end of pipe
		fflush(stdout);
		fflush(stderr);
		close(pipefd1[1]);
		close(pipefd2[1]);
		dup2(saved_stdout, STDOUT_FILENO); // return stdout to original fd
		dup2(saved_stderr, STDERR_FILENO); // return stderr to original fd
	}
	else { // fork failed
		write(1, "First Fork Failed, Exiting.\n", 28); 
		return -1;
	}


	pid = fork();
	
	if (pid == 0) { // child process - Execute T2
		fflush(stdout); // using flush here b/c somehow execv is calling before stdout prints to terminal

		// Determine what's requested
		int agent_perf_request = 0;  // 0: not requested, 1: stdout, 2: stderr
		int state_perf_request = 0;  // 0: not requested, 1: stdout, 2: stderr

		for (int i = 0; i < argc-1; i++) {
			if (strcmp(functionality[i], "agent_performance") == 0) {
				agent_perf_request = (strcmp(stream[i], "stdout") == 0) ? 1 : 2;
			}
			else if (strcmp(functionality[i], "state_performance") == 0) {
				state_perf_request = (strcmp(stream[i], "stdout") == 0) ? 1 : 2;
			}
		}

		int devnull = open("/dev/null", O_WRONLY);
		int saved_stdout = dup(STDOUT_FILENO);
		int saved_stderr = dup(STDERR_FILENO);

		if (agent_perf_request == 0) {
			dup2(devnull, STDOUT_FILENO);
		} else if (agent_perf_request == 2) {
			dup2(saved_stderr, STDOUT_FILENO);
		}

		if (state_perf_request == 0) {
			dup2(devnull, STDERR_FILENO);
		} else if (state_perf_request == 1) {
			dup2(saved_stdout, STDERR_FILENO);
		}

		close(saved_stdout);
		close(saved_stderr);
		//close(devnull);

		dup2(pipefd1[0], STDIN_FILENO); // redirect stdin to read end of pipe
		close(pipefd1[0]); // close original read-end of pipe fd

		char *args[] = {"./Transformer2", NULL};
		execv("./Transformer2", args);
	}

	else if (pid != -1) { // parent process - wait and break;
		int status;
		waitpid(pid, &status, 0);

		if (!WIFEXITED(status)) { // false if child process error
			fprintf(stdout, "error executing child process: T2.\n");
		}
		dup2(1, STDOUT_FILENO);
		dup2(2, STDERR_FILENO);
	}
	
	else { // fork failed
		write(1, "Second Fork Failed, Exiting.\n", 28); 
		return -1;
	}

	pid = fork();

	if (pid == 0) { // child process - Execute T3
		// determine functionalities and streams
		fflush(stderr);

		// Determine what's requested
		int agent_rate_request = 0;  // 0: not requested, 1: stdout, 2: stderr
		int state_rate_request = 0;  // 0: not requested, 1: stdout, 2: stderr

		for (int i = 0; i < argc-1; i++) {
			if (strcmp(functionality[i], "agent_rating") == 0) {
				agent_rate_request = (strcmp(stream[i], "stdout") == 0) ? 1 : 2;
			}
			else if (strcmp(functionality[i], "state_rating") == 0) {
				state_rate_request = (strcmp(stream[i], "stdout") == 0) ? 1 : 2;
			}
		}

		int devnull = open("/dev/null", O_WRONLY);
		int saved_stdout = dup(STDOUT_FILENO);
		int saved_stderr = dup(STDERR_FILENO);

		if (agent_rate_request == 0) {
			dup2(devnull, STDOUT_FILENO);
		} else if (agent_rate_request == 2) {
			dup2(saved_stderr, STDOUT_FILENO);
		}

		if (state_rate_request == 0) {
			dup2(devnull, STDERR_FILENO);
		} else if (state_rate_request == 1) {
			dup2(saved_stdout, STDERR_FILENO);
		}

		close(saved_stdout);
		close(saved_stderr);
		close(devnull);
		dup2(pipefd2[0], STDIN_FILENO);
		close(pipefd2[0]);

		if (agent_rate_request || state_rate_request) {
			char *args[] = {"./Transformer3", NULL};
			execv("./Transformer3", args);
		}
	}

	else if (pid != -1) { // parent process - wait and break;
		int status;
		waitpid(pid, &status, 0);

		if (!WIFEXITED(status)) { // false if child process error
			fprintf(stdout, "error executing child process: T3.\n");
			return -1;	
		}

	}

	else { // fork failed
		write(1, "Third Fork Failed, Exiting.\n", 28); 
		return -1;
	}

	close(pipefd2[0]);
	return 0;
}

void parse_arguments(int argc, char *argv[], char functionality[][32], char stream[][12]) {
	char *colon_ptr = NULL;
	int func_len;

	for (int i = 1; i < argc; i++) {
		// Find the colon delimiter
		colon_ptr = strchr(argv[i], ':');
		
		if (colon_ptr == NULL) {
			fprintf(stderr, "Error: Could not find colon: '%s'. Expected format: functionality:stream_name\n", argv[i]);
			exit(1);
		}
		
		// grab length of functionality and copy
		func_len = colon_ptr - argv[i];
		strncpy(stream[i-1], argv[i], func_len);
		functionality[i-1][func_len] = '\0';
		
		// copy stream after colon, already null terminated
		strcpy(functionality[i-1], colon_ptr + 1);
	}
}
