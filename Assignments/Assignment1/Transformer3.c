#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <string.h>

typedef struct {
	char agent_id[10];
	char rating[5];
	double rating_d;
}Agent;

typedef struct {
	char location[3];
	char rating[5];
	double rating_d;
}State;

void read_rating(char *buf, char *s, char *rating);

int main(int argc, char *argv[]) {

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

	char *s = NULL;
	FILE *fp = fdopen(STDIN_FILENO, "r");
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		int fields_idx = 0;
		int char_idx = 0;
		int comma_cnt = 0;
		s = buf;	
		
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
		read_rating(buf, s, rating);

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

	for (int i = 0; i < agent_count; i++) {
		fprintf(stdout, "%s, %s",  agents[i].agent_id, agents[i].rating);
	}

	for (int i = 0; i < state_count; i++) {
		fprintf(stderr, "%s, %s", states[i].location, states[i].rating);
	}
	free(agents);
	free(states);

    return 0;
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
