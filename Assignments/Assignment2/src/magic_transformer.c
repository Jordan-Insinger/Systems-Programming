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

#define BUF_SIZE 5

typedef struct {
	char agent_name[20];
	char agent_id[10];
	char loss_gain[32]; 
	char rating[5];
	double loss_gain_d;
	double rating_d;
}Agent;

typedef struct {
	char location[3];
	char loss_gain[32];
	char rating[5];
	double loss_gain_d;
	double rating_d;
}State;

typedef struct {
	char (*functionality)[32];
	char (*stream)[12];
	int count;
}ThreadArgs;

static pthread_mutex_t mtx_1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_2 = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t empty_cond_1 = PTHREAD_COND_INITIALIZER;
static pthread_cond_t empty_cond_2 = PTHREAD_COND_INITIALIZER;
static pthread_cond_t full_cond_1 = PTHREAD_COND_INITIALIZER;
static pthread_cond_t full_cond_2 = PTHREAD_COND_INITIALIZER;

static char *buffer_1[BUF_SIZE];
static char *buffer_2[BUF_SIZE];
static int num_items_1 = 0;
static int num_items_2 = 0;
static int in_1 = 0;
static int in_2 = 0;
static int out_1 = 0;
static int out_2 = 0;

void parse_arguments(int argc, char *argv[], char functionality[][32], char stream[][12]);
void read_price(char *buf, char **s, char **price);
void read_rating(char *buf, char *s, char *rating);
void format_price(char *price);

void *transformer1(void *args);
void *transformer2(void *args);
void *transformer3(void *args);

int main(int argc, char **argv) {

	if (argc == 1) {
		fprintf(stdout, "No output requested, have a nice day!\n");
		return 0;
	}
	
	char functionality[argc-1][32];
	char stream[argc-1][12];

	// parse arguments
	parse_arguments(argc, argv, functionality, stream);

	pthread_t threads[3];
	pthread_attr_t thread_attributes;
	pthread_attr_init(&thread_attributes);
	pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_JOINABLE);

	ThreadArgs thread_args;
	thread_args.count = argc-1;
	thread_args.functionality = functionality;
	thread_args.stream = stream;

	// create thread 1 to execute transformer 1
	pthread_create(&threads[0], &thread_attributes, transformer1, &thread_args);

	// create thread 2 to execute transformer 2
	pthread_create(&threads[1], &thread_attributes, transformer2, &thread_args);

	// create thread 3 to execute transformer 3
	pthread_create(&threads[2], &thread_attributes, transformer3, &thread_args);

	for (int i = 0; i < 3; i++) {
		pthread_join(threads[i], NULL);
	}

	return 0;
}



void *transformer1(void *args) {
	char *agent_name = malloc(10 * sizeof(char));
	char *agent_id = malloc(5 * sizeof(char));
	char *transaction_id = malloc(10 * sizeof(char));
	char *location = malloc(2 * sizeof(char));
	char *original_price = malloc(14*sizeof(char));
	char *sale_price = malloc(14*sizeof(char));
	char *customer_rating = malloc(3*sizeof(char));
	char *loss_gain = malloc(20*sizeof(char));
	int fields_idx;
	int char_idx;
	int comma_cnt;
	int comma_count;
	char buf[256];
	char fields[7][15];

	// read from stdin back line-by-line - while loop
	FILE *fp = fdopen(STDIN_FILENO, "r");
	
	char *s = NULL; 
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		fields_idx = 0;
		char_idx = 0;
		comma_cnt = 0;
		s = buf;

		if (s == NULL) return 0;

		// parse first 4 fields (agent name/id, transaction id, location)
		while (comma_cnt < 4) {
			if (*s == ',') {
				fields[fields_idx][char_idx] = '\0';

				fields_idx++;
				comma_cnt++;
				char_idx = 0;
				s++;
				
			}
			else {
				fields[fields_idx][char_idx] = *s;
				char_idx++;

			}
			s++;
		}
		
		agent_name = fields[0];
		agent_id = fields[1];
		transaction_id = fields[2];
		location = fields[3];

		read_price(buf, &s, &original_price);
		read_price(buf, &s, &sale_price);
		double rating_d = atof(s);
		snprintf(customer_rating, sizeof(customer_rating), "%.1f", rating_d);

		char *endptr;
		double original_price_d = strtod(original_price, &endptr);
		double sale_price_d = strtod(sale_price, &endptr);
		double diff = sale_price_d - original_price_d;

		if (diff == 0) {
			strcpy(loss_gain, "0.00");
		} else {
			loss_gain[0] = (diff > 0) ? '+' : '-';

			// price_to_string
			double absolute_diff = fabs(diff);
			snprintf(loss_gain + 1, 19, "%.2f", absolute_diff);
			format_price(loss_gain + 1);
		}

		if (original_price == 0) strcpy(original_price, "0.00");
		else format_price(original_price);

		if (sale_price_d == 0) strcpy(sale_price, "0.00");
		else format_price(sale_price);

		// output to stdout
		char *performance_data = malloc(256 * sizeof(char));
		char *rating_data = malloc(128 * sizeof(char));

		// format into performance and rating data respectively
		snprintf(performance_data, 256, "%s, %s, %s, %s, %s, %s\n",agent_name, agent_id, transaction_id, location, sale_price, loss_gain);
		snprintf(rating_data, 128, "%s, %s, %s\n", agent_id, location, customer_rating);

		// ============ CRITICAL REGION 1==============
		pthread_mutex_lock(&mtx_1); // lock mutex 1

		while (num_items_1 == BUF_SIZE) { // check and wait on full condition
			pthread_cond_wait(&full_cond_1, &mtx_1);
		}
		buffer_1[in_1] = performance_data; // write to buffer 1
		in_1 = (in_1 + 1) % BUF_SIZE;
		num_items_1++;

		if (num_items_1 == 1)
			pthread_cond_signal(&empty_cond_1); // signal condition variable

		pthread_mutex_unlock(&mtx_1); // unlock mutex 1
		// ===========================================

		// ============ CRITICAL REGION 2==============
		pthread_mutex_lock(&mtx_2); // lock mutex 2

		while (num_items_2 == BUF_SIZE) { // check and wait on full condition
			pthread_cond_wait(&full_cond_2, &mtx_2);
		}

		buffer_2[in_2] = rating_data; // write to buffer 2
		in_2 = (in_2 + 1) % BUF_SIZE;
		num_items_2++;

		if (num_items_2 == 1)
			pthread_cond_signal(&empty_cond_2); // signal condition variable

		pthread_mutex_unlock(&mtx_2); // unlock mutex 2
		// ===========================================
	}

	// send EOF signal
	pthread_mutex_lock(&mtx_1);

	while (num_items_1 == BUF_SIZE) { // check and wait on full condition
		pthread_cond_wait(&full_cond_1, &mtx_1);
	}

	buffer_1[in_1] = NULL;
	num_items_1++;

	if (num_items_1 == 1)
		pthread_cond_signal(&empty_cond_1); // signal condition variable

	pthread_mutex_unlock(&mtx_1);
	
	// send EOF signal 2
	pthread_mutex_lock(&mtx_2);

	while (num_items_2 == BUF_SIZE) { // check and wait on full condition
		pthread_cond_wait(&full_cond_2, &mtx_2);
	}

	buffer_2[in_2] = NULL;
	num_items_2++;

	if (num_items_2 == 1)
		pthread_cond_signal(&empty_cond_2); // signal condition variable

	pthread_mutex_unlock(&mtx_2);

	pthread_exit(NULL);
}

void *transformer2(void *args) {

	ThreadArgs *thread_args = (ThreadArgs *)args;

	// first, determine functionality -> stream mappings
	int agent_perf_request = 0;  // 0: not requested, 1: stdout, 2: stderr
	int state_perf_request = 0;  // 0: not requested, 1: stdout, 2: stderr

	for (int i = 0; i < thread_args->count; i++) {
		if (strcmp(thread_args->functionality[i], "agent_performance") == 0) {
			agent_perf_request = (strcmp(thread_args->stream[i], "stdout") == 0) ? 1 : 2;
		}
		else if (strcmp(thread_args->functionality[i], "state_performance") == 0) {
			state_perf_request = (strcmp(thread_args->stream[i], "stdout") == 0) ? 1 : 2;
		}
	}
	
	int comma_count;
	char *agent_name = malloc(10 * sizeof(char));
	char *agent_id = malloc(5 * sizeof(char));
	char *transaction_id = malloc(10 * sizeof(char));
	char *location = malloc(2 * sizeof(char));
	char *sale_price = malloc(14*sizeof(char));
	char *loss_gain = malloc(32*sizeof(char));
	Agent *agents = NULL;
	State *states = NULL;
	int agent_capacity = 0;
	int agent_count = 0;
	int state_count = 0;
	int state_capacity = 0;
	char buf[2048];
	char fields[6][20];

	for (;;) {
		pthread_mutex_lock(&mtx_1); // lock mutex 

		while (num_items_1 == 0) {
			pthread_cond_wait(&empty_cond_1, &mtx_1);
		}

		char *data = buffer_1[out_1];

		if (data == NULL) {
			pthread_mutex_unlock(&mtx_1);
			break; // EOF signal
		}

		char local_data[256];
		strcpy(local_data, data);
		free(data);

		out_1 = (out_1 + 1) % BUF_SIZE;
		num_items_1--;

		if (num_items_1 == BUF_SIZE - 1)
			pthread_cond_signal(&full_cond_1);

		pthread_mutex_unlock(&mtx_1);

		// RUN TRANSFORMER 2 OPERATIONS
		char *s = NULL;

		int fields_idx = 0;
		int char_idx = 0;
		int comma_cnt = 0;
		s = local_data;
		
		while (comma_cnt < 4) {
			if (*s == ',') {
				fields[fields_idx][char_idx] = '\0';

				fields_idx++;
				comma_cnt++;
				char_idx = 0;
				s++;
				
			}
			else {
				fields[fields_idx][char_idx] = *s;
				char_idx++;

			}
			s++;
		}

		agent_name = fields[0];
		agent_id = fields[1];
		transaction_id = fields[2];
		location = fields[3];

		read_price(local_data, &s, &sale_price);
		format_price(sale_price);

		// store loss/gain so I don't have to reformat after comparisons
		char temp_loss_gain[32];
		if (*s == '+') s++;
		strcpy(temp_loss_gain, s);
		read_price(local_data, &s, &loss_gain);

		char *endptr;
		double loss_gain_d = strtod(temp_loss_gain, &endptr);
		
		// add state to struct
		int found = -1;
		for (int i = 0; i < state_count; i++) {
			if  (strcmp(states[i].location, location) == 0) {
				found = i;
				break;
			}
		}

		// state found, compare earnings
		if (found != -1) {
			if (fabs(states[found].loss_gain_d) < fabs(loss_gain_d)) {
				states[found].loss_gain_d = loss_gain_d;
				strcpy(states[found].loss_gain, temp_loss_gain);
			}
		}

		// otherwise add to struct
		else {
			if (state_count >= state_capacity) { // increase state_capacity
				state_capacity = (state_capacity == 0) ? 10 : state_capacity * 2;
				states = realloc(states, state_capacity * sizeof(State));

				if (states == NULL) {
					fprintf(stderr, "Memory allocation failed\n");
					exit(1);
				}

			}
			// place in struct
			strcpy(states[state_count].location, location);
			strcpy(states[state_count].loss_gain, temp_loss_gain);
			states[state_count].loss_gain_d = loss_gain_d;
			state_count++;
		}


		found = -1;
		// add agent to struct
		for (int i = 0; i < agent_count; i++) { // check if agent exists
			if(strcmp(agents[i].agent_name, agent_name) == 0) {
				found = i;
				break;
			}
		}

		// agent found - compare loss/gain
		if (found != -1) {
			if (fabs(agents[found].loss_gain_d) < fabs(loss_gain_d)) {
				agents[found].loss_gain_d = loss_gain_d;
				strcpy(agents[found].loss_gain, temp_loss_gain);
			}
		}
		
		else { // agent not found check if we have space to insert

			if (agent_count >= agent_capacity) { // increase agent_capacity
				agent_capacity = (agent_capacity == 0) ? 10 : agent_capacity * 2;
				agents = realloc(agents, agent_capacity * sizeof(Agent));

				if (agents == NULL) {
					fprintf(stdout, "Memory allocation failed\n");
					exit(1);
				}

			}
			// place in struct
			strcpy(agents[agent_count].agent_name, agent_name);
			strcpy(agents[agent_count].agent_id, agent_id);
			agents[agent_count].loss_gain_d = loss_gain_d;
			strcpy(agents[agent_count].loss_gain, temp_loss_gain);
			agent_count++;
		}
	}

	if (agent_perf_request != 0) {
		if (agent_perf_request == 1) {
			for (int i = 0; i < agent_count; i++) {
				fprintf(stdout, "%s, %s, %s", agents[i].agent_name, agents[i].agent_id, agents[i].loss_gain);
			}

		}
		else if (agent_perf_request == 2) {
			for (int i = 0; i < agent_count; i++) {
				fprintf(stderr, "%s, %s, %s", agents[i].agent_name, agents[i].agent_id, agents[i].loss_gain);
			}
		}
	}
	fflush(stdout);
	fflush(stderr);

	if (state_perf_request != 0) {
		if (state_perf_request == 1) {
			for (int i = 0; i < state_count; i++) {
				fprintf(stdout, "%s, %s", states[i].location, states[i].loss_gain);
			}
		}
		else if (state_perf_request == 2) {
			for (int i = 0; i < state_count; i++) {
				fprintf(stderr, "%s, %s", states[i].location, states[i].loss_gain);
			}
		}
	}
	fflush(stdout);
	fflush(stderr);

	free(agents);
	free(states);
	pthread_exit(NULL);
}

void *transformer3(void *args) {

	ThreadArgs *thread_args = (ThreadArgs *)args;

	// first, determine functionality -> stream mappings
	int agent_rating_request = 0;  // 0: not requested, 1: stdout, 2: stderr
	int state_rating_request = 0;  // 0: not requested, 1: stdout, 2: stderr

	for (int i = 0; i < thread_args->count; i++) {
		if (strcmp(thread_args->functionality[i], "agent_rating") == 0) {
			agent_rating_request = (strcmp(thread_args->stream[i], "stdout") == 0) ? 1 : 2;
		}
		else if (strcmp(thread_args->functionality[i], "state_rating") == 0) {
			state_rating_request = (strcmp(thread_args->stream[i], "stdout") == 0) ? 1 : 2;
		}
	}


	int comma_count;
	char *agent_id = malloc(5 * sizeof(char));
	char *location = malloc(2 * sizeof(char));
	char *rating = malloc(3*sizeof(char));
	int agent_capacity = 0;
	int agent_count = 0;
	int state_count = 0;
	int state_capacity = 0;
	Agent *agents = NULL;
	State *states = NULL;
	char buf[2048];
	char fields[3][20];
	int fields_idx = 0;
	int char_idx = 0;
	int comma_cnt = 0;
	char *s = NULL;

	for (;;) {
		pthread_mutex_lock(&mtx_2); // lock mutex 

		while (num_items_2 == 0) {
			pthread_cond_wait(&empty_cond_2, &mtx_2);
		}

		char *data = buffer_2[out_2];

		if (data == NULL) {
			pthread_mutex_unlock(&mtx_2);
			break; // EOF signal
		}

		char local_data[256];
		strcpy(local_data, data);
		free(data);
		
		out_2 = (out_2 + 1) % BUF_SIZE;
		num_items_2--;

		if (num_items_2 == BUF_SIZE - 1)
			pthread_cond_signal(&full_cond_2);

		pthread_mutex_unlock(&mtx_2);

		fields_idx = 0;
		char_idx = 0;
		comma_cnt = 0;
		s = local_data;

		while (comma_cnt < 2) {
			if (*s == ',') {
				fields[fields_idx][char_idx] = '\0';

				fields_idx++;
				comma_cnt++;
				char_idx = 0;
				s++;
				
			}
			else {
				fields[fields_idx][char_idx] = *s;
				char_idx++;

			}
			s++;
		}

		agent_id = fields[0];
		location = fields[1];


		// store loss/gain so I don't have to reformat after comparisons
		char temp_rating[5];
		strcpy(temp_rating, s);
		read_rating(local_data, s, rating);

		char *endptr;
		double rating_d = strtod(rating, &endptr);


		// add state to struct
		int found = -1;
		for (int i = 0; i < state_count; i++) {
			if  (strcmp(states[i].location, location) == 0) {
				found = i;
				break;
			}
		}

		// state found, compare earnings
		if (found != -1) {
			if (fabs(states[found].rating_d) < fabs(rating_d)) {
				states[found].rating_d = rating_d;
				strcpy(states[found].rating, temp_rating);
			}
		}

		// otherwise add to struct
		else {
			if (state_count >= state_capacity) { // increase state_capacity
				state_capacity = (state_capacity == 0) ? 10 : state_capacity * 2;
				states = realloc(states, state_capacity * sizeof(State));

				if (states == NULL) {
					fprintf(stderr, "Memory allocation failed\n");
					exit(1);
				}

			}
			// place in struct
			strcpy(states[state_count].location, location);
			strcpy(states[state_count].rating, temp_rating);
			states[state_count].rating_d = rating_d;
			state_count++;
		}


		found = -1;
		// add agent to struct
		for (int i = 0; i < agent_count; i++) { // check if agent exists
			if(strcmp(agents[i].agent_id, agent_id) == 0) {
				found = i;
				break;
			}
		}

		// agent found - compare loss/gain
		if (found != -1) {
			if (fabs(agents[found].rating_d) < fabs(rating_d)) {
				agents[found].rating_d = rating_d;
				strcpy(agents[found].rating, temp_rating);
			}
		}
		
		else { // agent not found check if we have space to insert

			if (agent_count >= agent_capacity) { // increase agent_capacity
				agent_capacity = (agent_capacity == 0) ? 10 : agent_capacity * 2;
				agents = realloc(agents, agent_capacity * sizeof(Agent));

				if (agents == NULL) {
					fprintf(stdout, "Memory allocation failed\n");
					exit(1);
				}

			}
			// place in struct
			strcpy(agents[agent_count].agent_id, agent_id);
			agents[agent_count].rating_d = rating_d;
			strcpy(agents[agent_count].rating, temp_rating);
			agent_count++;
		}
	}

	if (agent_rating_request != 0) {
		if (agent_rating_request == 1) {
			for (int i = 0; i < agent_count; i++) {
				fprintf(stdout, "%s, %s",  agents[i].agent_id, agents[i].rating);
			}
		}
		else if (agent_rating_request == 2) {
			for (int i = 0; i < agent_count; i++) {
				fprintf(stderr, "%s, %s",  agents[i].agent_id, agents[i].rating);
			}
		}
	}
	fflush(stdout);
	fflush(stderr);

	if (state_rating_request != 0) {
		if (state_rating_request == 1) {
			for (int i = 0; i < state_count; i++) {
				fprintf(stdout, "%s, %s", states[i].location, states[i].rating);
			}
		}
		else if (state_rating_request == 2) {
			for (int i = 0; i < state_count; i++) {
				fprintf(stderr, "%s, %s", states[i].location, states[i].rating);
			}
		}
	}
	fflush(stdout);
	fflush(stderr);

	free(agents);
	free(states);
	
	pthread_exit(NULL);
}

void parse_arguments(int argc, char *argv[], char functionality[][32], char stream[][12]) {
	char *colon_ptr = NULL;
	int func_len;

	for (int i = 1; i < argc; i++) {
		colon_ptr = strchr(argv[i], ':'); // Find the colon delimiter
		
		if (colon_ptr == NULL) {
			fprintf(stderr, "Error: Could not find colon: '%s'. Expected format: functionality:stream_name\n", argv[i]);
			exit(1);
		}
		
		func_len = colon_ptr - argv[i]; // grab length of functionality and copy
		strncpy(functionality[i-1], argv[i], func_len);
		functionality[i-1][func_len] = '\0';
		strcpy(stream[i-1], colon_ptr + 1); // copy stream after colon, already null terminated

		// check if functionality is being directed to multiple streams
		for (size_t j = 0; j < i-1; j++) {
			if (strcmp(functionality[j], functionality[i-1]) == 0 && strcmp(stream[j], stream[i-1]) != 0) {
				fprintf(stdout, "Error: %s is directed to more than one output stream!\n", functionality[j]);
				exit(1);
			}
		}
	}
}

void read_price(char *buf, char **s, char **price) {
	int idx = 0;

	// price is 0, there is no comma or decimal
	if (**s == '0') {  
		(*price)[0] = '0';
		(*price)[1] = '\0';
		(*s) += 3;  
	}
	else if (**s == '+') (*s)++;
	else if (**s == '-') (*s)++;

	else {
		// otherwise, read until decimal
		while (**s != '.') {
			if (**s == ',') {
				(*s)++;
			}
			else {
				(*price)[idx] = **s;  
				(*s)++;
				idx++;
			}
		}
		
		// reached decimal, read next two chars
		(*price)[idx] = **s;  
		idx++;
		(*s)++;
		for (int i = 0; i < 2; i++) {
			(*price)[idx] = **s;
			idx++;
			(*s)++;
		}
		(*price)[idx] = '\0';
		(*s) += 2;  // Skip the comma after the price
	}
}

void read_rating(char *buf, char *s, char *rating) {
	int idx = 0;

	// rating is 0, there is decimal
	if (*s == 0) {
		rating[0] = '0';
		rating[1] = '\0';
	}

	// otherwise read 3 characters
	else {
		for (int i = 0; i < 3; i++) {
			rating[idx] = *s;
			idx++;
			s++;
		}
		rating[idx] = '\0';
	}
}

void format_price(char *price) {
    char temp[20];
    int temp_idx = 0;
    int price_len = strlen(price);
    int digits_before_decimal = price_len - 3; 
    
    if (strcmp(price, "0.00") == 0) return;

    if (digits_before_decimal <= 3) {
        return;  
    }
    
    int digits_seen = 0;
    for (int i = 0; i < price_len; i++) {
        if (i < digits_before_decimal) {
            // Add comma every 3 digits
            if (digits_seen > 0 && (digits_before_decimal - i) % 3 == 0) {
                temp[temp_idx++] = ',';
            }
            temp[temp_idx++] = price[i];
            digits_seen++;
        } else {
            // Copy decimal point and cents
            temp[temp_idx++] = price[i];
        }
    }
    temp[temp_idx] = '\0';
    strcpy(price, temp);
}
