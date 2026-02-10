#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <string.h>

typedef struct {
	char agent_name[20];
	char agent_id[10];
	char loss_gain[32]; 
	double loss_gain_d;
}Agent;

typedef struct {
	char location[3];
	char loss_gain[32];
	double loss_gain_d;
}State;

void read_price(char *buf, char **s, char **price);
void format_loss_gain(char *buf, char **s, char **price); 
void format_price(char *price);

int main(int argc, char *argv[]) {
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

	char *s = NULL;
	FILE *fp = fdopen(STDIN_FILENO, "r");
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		int fields_idx = 0;
		int char_idx = 0;
		int comma_cnt = 0;
		s = buf;	
		
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

		read_price(buf, &s, &sale_price);
		format_price(sale_price);

		// store loss/gain so I don't have to reformat after comparisons
		char temp_loss_gain[32];
		if (*s == '+') s++;
		strcpy(temp_loss_gain, s);
		read_price(buf, &s, &loss_gain);

		char *endptr;
		double loss_gain_d = strtod(loss_gain, &endptr);












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

	// TODO: if agent performance arg == stdout, 
	// TODO: else if agent performance arg == stderr
	for (int i = 0; i < agent_count; i++) {
		fprintf(stdout, "%s, %s, %s", agents[i].agent_name, agents[i].agent_id, agents[i].loss_gain);
	}
	fflush(stdout);

	// TODO: if state performance arg == stdout, 
	// TODO: else if state performance arg == stderr
	for (int i = 0; i < state_count; i++) {
		fprintf(stderr, "%s, %s", states[i].location, states[i].loss_gain);
	}
	fflush(stderr);
	free(agents);
	free(states);

    return 0;
}

void format_price(char *price) {
    char temp[32];
    int temp_idx = 0;
    int price_len = strlen(price);
    int digits_before_decimal = price_len - 3; 
    
	if (price == "0.00") return;
	
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

void read_price(char *buf, char **s, char **price) {
	int idx = 0;

	if (**s == '+') (*s)++;
	else if (**s == '-') (*s)++;

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

void format_loss_gain(char *buf, char **s, char **price) {

}
