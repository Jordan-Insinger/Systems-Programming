#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

typedef struct {
	int account_from;
	int account_to;
	int amount;
} Transaction;

typedef struct {
	int account_no;
	int balance;
	pthread_mutex_t mutex;
} Account;

static void sig_handler(int sig) 
{
	fprintf(stdout, "no.\n");
}

void parse_arguments(int argc, char** argv, char* concurrency_mode, char* num_workers, char* signal_mode);

int main(int argc, char** argv) 
{

	// install handler for SIGINT
	void (*old_handler) = signal(SIGINT, sig_handler);
	if (old_handler == SIG_ERR)
		exit(1);

	char* concurrency_mode = malloc(15 * sizeof(char));
	char* num_workers_s = malloc(5 * sizeof(char));
	char* signal_mode = malloc(7 * sizeof(char));
	int num_workers = 0;

	parse_arguments(argc, argv, concurrency_mode, num_workers_s, signal_mode);
	num_workers = atoi(num_workers_s);
	
	Transaction* transactions_ = NULL;
	Account* accounts_ = NULL;

	int account_no, initial_balance, account_from, account_to, amount;
	int n_a = 0, n_t = 0, cap_a = 0, cap_t = 0;

	char line[256];
	while (fgets(line, sizeof(line), stdin))
	{

		if(strncmp(line, "Transfer", 8) == 0) 
		{
			sscanf(line, "Transfer %d %d %d", &account_from, &account_to, &amount);

			if (n_t == cap_t)
			{
				cap_t = cap_t ? cap_t * 2 : 4;
				transactions_ = realloc(transactions_, cap_t * sizeof(Transaction));
			}
			
			transactions_[n_t].account_from = account_from;
			transactions_[n_t].account_to = account_to;
			transactions_[n_t].amount = amount;
			n_t++;
		}

		else 
		{
			sscanf(line, "%d %d", &account_no, &initial_balance);

			if (n_a == cap_a)
			{
				cap_a = cap_a ? cap_a * 2 : 4;
				accounts_ = realloc(accounts_, cap_a * sizeof(Account));
			}

			accounts_[n_a].account_no = account_no;
			accounts_[n_a].balance = initial_balance;
			pthread_mutex_init(&accounts_[n_a].mutex, NULL);
			n_a++;
		}

	}

	for (size_t i = 0; i < n_a; i++)
	{
		fprintf(stdout, "Account - ID: %d, balance: %d\n", accounts_[i].account_no, accounts_[i].balance);
	}

	for (size_t i = 0; i < n_t; i++)
	{
		fprintf(stdout, "Transaction - to :%d, from: %d, amount: %d\n", transactions_[i].account_from,transactions_[i].account_to,transactions_[i].amount);
	}                               

	return 0;                       
}

void parse_arguments(int argc, char** argv, char* concurrency_mode, char* num_workers, char* signal_mode)
{
	strcpy(concurrency_mode, argv[1]);
	strcpy(num_workers, argv[2]);
	strcpy(signal_mode, argv[3]);
}
