#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
	struct thread_data* thread_func_args = (struct thread_data *) thread_param;
	usleep(thread_func_args->obtain_wait);
	int rc = pthread_mutex_lock(thread_func_args->mutex);
	if (rc != 0) {
		fprintf(stderr, "Error locking mutex: %d\n", rc);
		thread_func_args->thread_complete_success = false;
		return thread_param;
	}


	usleep(thread_func_args->release_wait);
	rc = pthread_mutex_unlock(thread_func_args->mutex);
	if (rc != 0) {
		fprintf(stderr, "Error unlocking mutex: %d\n", rc);
		thread_func_args->thread_complete_success = false;
		return thread_param;
	}

	thread_func_args->thread_complete_success = true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    struct thread_data* thread_data_ptr = (struct thread_data*)malloc(sizeof(struct thread_data));
	thread_data_ptr->obtain_wait = wait_to_obtain_ms;
	thread_data_ptr->release_wait = wait_to_release_ms;
	thread_data_ptr->mutex = mutex;

    int result = pthread_create(thread, NULL, threadfunc, thread_data_ptr);
	if (result != 0) {
		fprintf(stderr, "Error creating thread: %d\n", result);
		return false;
	}

    return true; 
}

