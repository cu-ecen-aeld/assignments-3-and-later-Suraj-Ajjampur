#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...) printf("INFO: " msg "\n" , ##__VA_ARGS__)

#define ERROR_LOG(msg,...) printf("ERROR: " msg "\n" , ##__VA_ARGS__)

#define SUCCESS 		(0)
#define ERROR 		(-1)

#define BACKLOG_CONNECTIONS	(10)

#define BUF_LEN		(1024)

/**
 * @struct status_flags
 * @brief Struct to hold various status flags.
 *
 * This struct is designed to hold flags that can be used to keep track of
 * various statuses within the program, such as whether a file or socket
 * is open, or if the program is in daemon mode.
 *
 * @var bool file_open
 * Indicates if the data file is currently open.
 *
 * @var bool log_open
 * Indicates if the syslog logging is currently open.
 *
 * @var bool socket_open
 * Indicates if the socket is currently open.
 *
 * @var bool client_fd_open
 * Indicates if the client file descriptor is currently open.
 *
 * @var bool signal_caught
 * Indicates if a signal has been caught for special handling.
 *
 * @var bool daemon_mode
 * Indicates if the program is currently running in daemon mode.
 *
 * @var bool command_status_success
 * Indicates if the last executed command was successful.
 */
typedef struct
{
    bool file_open;
    bool log_open;
    bool socket_open;
    bool client_fd_open;
    bool signal_caught;
    bool daemon_mode;
    bool command_status_success;
} status_flags;