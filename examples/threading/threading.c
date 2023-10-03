#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

/**
 * @brief This function is the entry point for the threads created by start_thread_obtaining_mutex.
 *
 * It sleeps for a specified amount of time, locks a mutex, sleeps again, releases the mutex, and sets
 * the thread_complete_success flag to true before exiting.
 *
 * @param thread_param A pointer to the thread_data structure containing thread-specific information.
 *
 * @return Returns the thread_param pointer when the thread exits.
 */
void* threadfunc(void* thread_param)
{
    // Cast the thread_param pointer to a struct thread_data pointer to access its members.
    // struct thread_data* data = (struct thread_data*)thread_param;
    unsigned int time_to_sleep = ((struct thread_data*)thread_param)->wait_to_obtain_ms;
    unsigned int time_to_hold = ((struct thread_data*)thread_param)->wait_to_release_ms;
    pthread_mutex_t* mutex = ((struct thread_data*)thread_param)->mutex;

    // Sleep for the specified time before attempting to obtain the mutex.
    if (usleep(time_to_sleep * 1000) == -1)
    {
        syslog(LOG_ERR, "usleep() failed: %s", strerror(errno));
        ((struct thread_data*)thread_param)->thread_complete_success = false;
        return thread_param;
    }

    // Attempt to lock the mutex.
    int ret = pthread_mutex_lock(mutex);
    if (ret != 0)
    {
        syslog(LOG_ERR, "pthread_mutex_lock() failed: %d", ret);
        ((struct thread_data*)thread_param)->thread_complete_success = false;
        return thread_param;
    }

    // Sleep for the specified time while holding the mutex.
    if (usleep(time_to_hold * 1000) == -1)
    {
        syslog(LOG_ERR, "usleep() failed: %s", strerror(errno));
        // Unlock the mutex before returning to ensure it's not left locked.
        pthread_mutex_unlock(mutex);
        ((struct thread_data*)thread_param)->thread_complete_success = false;
        return thread_param;
    }

    // Release the mutex.
    ret = pthread_mutex_unlock(mutex);
    if (ret != 0)
    {
        syslog(LOG_ERR, "pthread_mutex_unlock() failed: %d", ret);
        ((struct thread_data*)thread_param)->thread_complete_success = false;
        return thread_param;
    }

    // Set thread_complete_success flag to true before exiting.
    ((struct thread_data*)thread_param)->thread_complete_success = true;

    return thread_param;
}

/**
 * @brief This function starts a new thread that sleeps for a specified time, obtains a mutex, holds
 * it for another specified time, and then releases it. The thread-specific information is stored in a
 * dynamically allocated thread_data structure.
 *
 * @param thread A pointer to a pthread_t variable where the thread ID will be stored.
 * @param mutex A pointer to the mutex that the thread will lock and release.
 * @param wait_to_obtain_ms The time in milliseconds the thread should sleep before obtaining the mutex.
 * @param wait_to_release_ms The time in milliseconds the thread should hold the mutex before releasing it.
 *
 * @return Returns true if the thread was successfully created, false otherwise.
 */
bool start_thread_obtaining_mutex(pthread_t* thread, pthread_mutex_t* mutex, int wait_to_obtain_ms, int wait_to_release_ms)
{
    // Allocate memory for thread_data structure and initialize its members.
    struct thread_data* thread_func_args = (struct thread_data*)malloc(sizeof(struct thread_data));
    thread_func_args->mutex = mutex;
    thread_func_args->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_func_args->wait_to_release_ms = wait_to_release_ms;

    // Create a new thread using threadfunc as the entry point and pass the thread_data structure.
    int ret = pthread_create(thread, NULL, threadfunc, thread_func_args);

    // Check if the thread creation was successful.
    if (ret == 0)
        return true;
    
    // Log an error message if pthread_create fails and return false.
    syslog(LOG_ERR, "pthread_create() failed: %s", strerror(ret));
    return false;
}

