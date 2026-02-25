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
	size_t account_from;
	size_t account_to;
	size_t amount;
} Transaction;

typedef struct {
	size_t account_no;
	size_t balance;
	pthread_mutex_t mutex;
} Account;

typedef struct {
	Transaction* transactions;
	Account* accounts;
	size_t thread_id;
	size_t n_t;
	size_t n_a;
	size_t num_workers;
}ThreadArgs;

static void sig_handler(int sig) 
{
	fprintf(stdout, "no.\n");
}

void read_input(Transaction** transactions_, Account** accounts_, size_t* n_t,  size_t* n_a); 
void parse_arguments(int argc, char** argv, char* concurrency_mode, char* num_workers, char* signal_mode);
void* execute_transactions(void* args);

int main(int argc, char** argv) 
{

	// install handler for SIGINT
	void (*old_handler) = signal(SIGINT, sig_handler);
	if (old_handler == SIG_ERR)
		exit(1);

	// initialize argument variables
	char* concurrency_mode = malloc(15 * sizeof(char));
	char* num_workers_s = malloc(5 * sizeof(char));
	char* signal_mode = malloc(7 * sizeof(char));
	size_t num_workers = 0;

	parse_arguments(argc, argv, concurrency_mode, num_workers_s, signal_mode);
	num_workers = atoi(num_workers_s);
	
	Transaction* transactions_ = NULL;
	Account* accounts_ = NULL;
	size_t n_a = 0;
	size_t n_t = 0;

	read_input(&transactions_, &accounts_, &n_t, &n_a);

	if (strcmp(concurrency_mode, "single_process") == 0) // create worker threads
	{
		pthread_t threads[num_workers];
		pthread_attr_t thread_attributes;
		size_t thread_id;

		pthread_attr_init(&thread_attributes);
		pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_JOINABLE);
		
		for(thread_id = 0; thread_id < num_workers; thread_id++)
		{
			ThreadArgs* args = malloc(sizeof(ThreadArgs));
			args->transactions = transactions_;
			args->accounts = accounts_;
			args->thread_id = thread_id;
			args->n_t = n_t;
			args->n_a = n_a;
			args->num_workers = num_workers;
			pthread_create(&threads[thread_id], &thread_attributes, execute_transactions, args);
		}

		for (thread_id = 0; thread_id < num_workers; thread_id++)
		{
			pthread_join(threads[thread_id], NULL);
		}

		for (size_t i = 0; i < n_a; i++)
		{
			fprintf(stdout, "%ld %ld\n", i+1, accounts_[i].balance);
		}

	}

	return 0;                       
}

void* execute_transactions(void* args)
{
	ThreadArgs* thread_args = (ThreadArgs*)args;
	size_t from_id;
	size_t to_id;
	size_t transaction_amount;
	
	for (size_t i = thread_args->thread_id; i < thread_args->n_t; ) {
		// grab account ids
		from_id = thread_args->transactions[i].account_from - 1;
		to_id = thread_args->transactions[i].account_to - 1;
		transaction_amount = thread_args->transactions[i].amount;

		//fprintf(stdout, "Thread %ld executing transaction: %ld, %ld -> %ld\n", thread_args->thread_id, transaction_amount, from_id+1, to_id+1);

		if (from_id < to_id) // grab mutex for account with lower id
		{
			pthread_mutex_lock(&thread_args->accounts[from_id].mutex);	
			pthread_mutex_lock(&thread_args->accounts[to_id].mutex);	
			thread_args->accounts[from_id].balance -= transaction_amount;
			thread_args->accounts[to_id].balance += transaction_amount;
		}
		else if (from_id > to_id)
		{
			pthread_mutex_lock(&thread_args->accounts[to_id].mutex);	
			pthread_mutex_lock(&thread_args->accounts[from_id].mutex);	
			thread_args->accounts[from_id].balance -= transaction_amount;
			thread_args->accounts[to_id].balance += transaction_amount;
		}

		// unlock both mutexes
		pthread_mutex_unlock(&thread_args->accounts[from_id].mutex);	
		pthread_mutex_unlock(&thread_args->accounts[to_id].mutex);

		i += thread_args->num_workers;
	}

	pthread_exit((void*) thread_args);
}

void read_input(Transaction** transactions_, Account** accounts_, size_t* n_t,  size_t* n_a)
{
	int account_no, initial_balance, account_from, account_to, amount;
	int cap_a = 0, cap_t = 0;

	char line[256];
	while (fgets(line, sizeof(line), stdin))
	{

		if(strncmp(line, "Transfer", 8) == 0) 
		{
			sscanf(line, "Transfer %d %d %d", &account_from, &account_to, &amount);

			if (*n_t == cap_t)
			{
				cap_t = cap_t ? cap_t * 2 : 4;
				*transactions_ = realloc(*transactions_, cap_t * sizeof(Transaction));
			}
			
			(*transactions_)[*n_t].account_from = account_from;
			(*transactions_)[*n_t].account_to = account_to;
			(*transactions_)[*n_t].amount = amount;
			(*n_t)++;
		}

		else 
		{
			sscanf(line, "%d %d", &account_no, &initial_balance);

			if (*n_a == cap_a)
			{
				cap_a = cap_a ? cap_a * 2 : 4;
				*accounts_ = realloc(*accounts_, cap_a * sizeof(Account));
			}

			(*accounts_)[*n_a].account_no = account_no;
			(*accounts_)[*n_a].balance = initial_balance;
			pthread_mutex_init(&(*accounts_)[*n_a].mutex, NULL);
			(*n_a)++;
		}

	}
}

void parse_arguments(int argc, char** argv, char* concurrency_mode, char* num_workers, char* signal_mode)
{
	if (argc == 1)
	{
		fprintf(stdout, "No arguments provided\n");
		exit(1);
	}

	strcpy(concurrency_mode, argv[1]);
	strcpy(num_workers, argv[2]);
	strcpy(signal_mode, argv[3]);

}
