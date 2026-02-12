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

static pthread_mutex_t mtx_1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_2 = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t empty_cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t full_cond = PTHREAD_COND_INITIALIZER;

static char *buffer_1[BUF_SIZE];
static char *buffer_2[BUF_SIZE];
static int num_items = 0;

void parse_arguments(int argc, char *argv[], char functionality[][32], char stream[][12]);
void read_price(char *buf, char **s, char **price);
void read_rating(char *buf, char *s, char *rating);
void format_price(char *price);

void *transformer1();
void *transformer2();
void *transformer3();

int main(int argc, char **argv) {


	if (argc == 1) {
		fprintf(stdout, "No output requested, have a nice day!\n");
		return 0;
	}
	

	char functionality[argc-1][32];
	char stream[argc-1][12];

	pthread_t threads[3];
	pthread_attr_t thread_attributes;
	pthread_attr_init(&thread_attributes);
	pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_JOINABLE);

	

	// parse arguments
	parse_arguments(argc, argv, functionality, stream);

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
		char performance_data[256];
		char rating_data[128];

		// format into performance and rating data respectively
		snprintf(performance_data, sizeof(performance_data), "%s, %s, %s, %s, %s, %s\n",agent_name, agent_id, transaction_id, location, sale_price, loss_gain);
		snprintf(rating_data, sizeof(rating_data), "%s, %s, %s\n", agent_id, location, customer_rating);

		// ============ CRITICAL REGION ==============
		pthread_mutex_lock(&mtx_1); // lock mutex 1

		while (num_items == BUF_SIZE) { // check and wait on full condition
			pthread_cond_wait(&full_cond, &mtx_1);
		}

		buffer_1[num_items] = performance_data; // write to buffer 1
		num_items++;

		if (num_items == 1)
			pthread_cond_signal(&empty_cond); // signal condition variable

		pthread_mutex_unlock(&mtx_1); // unlock mutex 1
		// ===========================================
	}
	
	// lock mutex 2
	// check full condition variable in while loop
	// condition wait if not ready
	// write to buffer 2
	// signal empty condition variable
	// unlock mutex 2
	
	pthread_exit(NULL);
}

void *transformer2() {
	fprintf(stdout, "thread2 created\n");

	for (;;) {
		pthread_mutex_lock(&mtx_1); // lock mutex 

		while (num_items == 0) {
			pthread_cond_wait(&empty_cond, &mtx_1);
		}

		fprintf(stdout, "%s", buffer_1[num_items-1]);
		num_items--;

		if (num_items == BUF_SIZE - 1)
			pthread_cond_signal(&full_cond);

		pthread_mutex_unlock(&mtx_1);
	}

	pthread_exit(NULL);
}

void *transformer3() {
	fprintf(stdout, "thread3 created\n");
	pthread_exit(NULL);
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

void read_price(char *buf, char **s, char **price) {
	int idx = 0;

	// price is 0, there is no comma or decimal
	if (**s == '0') {  
		(*price)[0] = '0';
		(*price)[1] = '\0';
		(*s) += 3;  
	}
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
