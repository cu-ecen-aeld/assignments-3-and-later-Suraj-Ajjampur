#include "systemcalls.h"
#include <stdlib.h>  // Standard library functions (for exit)
#include <syslog.h>  // Syslog functions for logging
#include <unistd.h>  // UNIX standard function definitions
#include <sys/types.h> // Data types (e.g., pid_t)
#include <sys/wait.h>  // Process wait functions (e.g., wait)
#include <fcntl.h>    //Control Operations
#include <string.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/

bool do_system(const char *cmd) {
    // Create a pipe to capture the command's output
    int status = system(cmd);
    
    if(-1 == status){
	 //Log error
	  return false;
    }
	
    if (WIFEXITED(status)) {
        // Child process exited normally
        return (WEXITSTATUS(status) == 0);
    } 
    else 
    {
            // Child process did not exit normally
            return false;
    }

    return true;
}

bool execute_command(char * command[], int redirect_fd)
{
    pid_t child_pid = fork();

    if (child_pid == -1) {
        // Fork failed
        syslog(LOG_ERR, "Fork failed");
        return false;
    }

    if (child_pid == 0) {
        // This is the child process

        // Redirect stdout if necessary
        if (redirect_fd != -1) {
            if (dup2(redirect_fd, 1) == -1) {
                syslog(LOG_ERR, "Failed to redirect stdout");
                exit(EXIT_FAILURE);
            }
        }

        // Execute the command
        if (execv(command[0], command) == -1) {
            syslog(LOG_ERR, "execv failed");
            exit(EXIT_FAILURE);
        }
    } else {
        // This is the parent process
        int status;

        // Wait for the child process to finish
        wait(&status);

        if (WIFEXITED(status)) {
            return (WEXITSTATUS(status) == 0);
        } else {
            return false;
        }
    }

    return true;  // Should never reach here
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
//The function initializes a variable argument list args using va_start to process the variadic arguments.
    va_list args;
    va_start(args, count);
    //It then creates an array command to store the command and its arguments. The array size is set to count + 1 to accommodate the command and a NULL pointer (as required by execv()).
    char * command[count+1];
    int i;

    // It loops through the variadic arguments, extracting each argument and storing it in the command array
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    //Sets the last element of array to NULL to terminal the argument list
    command[count] = NULL;
    
    va_end(args);

    //Calling execute_command to avoid duplicating functionality
    return execute_command(command, -1);
	/*
	 * TODO:
	 *   Execute a system command by calling fork, execv(),
	 *   and wait instead of system (see LSP page 161).
	 *   Use the command[0] as the full path to the command to execute
	 *   (first argument to execv), and use the remaining arguments
	 *   as second argument to the execv() command.
	 *
	*/
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    // Create a file descriptor for the output file
    int output_fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (output_fd == -1) {
        // Opening the output file failed
        openlog("MyProgram", LOG_PID | LOG_CONS, LOG_USER);
        syslog(LOG_ERR, "Failed to open output file");
        closelog();
        va_end(args);
        return false;
    }

    bool result = execute_command(command, output_fd);
    close(output_fd);

    return result;
}


