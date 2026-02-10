#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

void read_price(char *buf, char **s, char **price);
void read_rating(char *buf, char *s, char *rating);
void format_price(char *price);

int main(int argc, char *argv[]) {

	char buf[256];
	int comma_count;
	char *agent_name = malloc(10 * sizeof(char));
	char *agent_id = malloc(5 * sizeof(char));
	char *transaction_id = malloc(10 * sizeof(char));
	char *location = malloc(2 * sizeof(char));
	char *original_price = malloc(14*sizeof(char));
	char *sale_price = malloc(14*sizeof(char));
	char *customer_rating = malloc(3*sizeof(char));
	char *loss_gain = malloc(20*sizeof(char));
	char fields[7][15];
	int fields_idx;
	int char_idx;
	int comma_cnt;

	FILE *fp = fdopen(STDIN_FILENO, "r");

	char *s = NULL; // character to iterate over the line that fgets reads in
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
		fprintf(stdout, "%s, %s, %s, %s, %s, %s\n",agent_name, agent_id, transaction_id, location, sale_price, loss_gain);
		fprintf(stderr, "%s, %s, %s\n", agent_id, location, customer_rating);
	}

	return 0;
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
