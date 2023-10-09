/***********************************************************************
 * @file      		aesdsocket.c
 * @version   		0.1
 * @brief		    Socket server application code
 *
 * @author    		Suraj Ajjampur, suraj.ajjampur@Colorado.edu
 * @date      		Oct 8, 2023
 *
 *
 * @reference
 *
 * https://www.thegeekstuff.com/2012/02/c-daemon-process/ 
 ************************************************************************/

/****************   Includes    ***************/ 
#include "aesdsocket.h"

/****************   Macros     ***************/ 
#define DATA_FILE "/var/tmp/aesdsocketdata"

/****************   Global Variables     ***************/ 
sig_atomic_t fatal_error_in_progress = 0;

// Outout data file
int dataFileDescriptor;

// Server & Client Socket
int sock_fd;
int clientSocketFd;

/****************   Function prototypes     ***************/ 
void cleanup_on_exit();
void main_socket_application();
int run_daemon(void);
int accept_and_log_client(char *ip_address);
int receive_and_store_data(void);
int send_file_content_to_client(void);


// Initialize all elements to false
status_flags s_flags = {false, false, false, false, false, false, false};

/**
 * @name handle_termination
 * 
 * @brief Handler function to cause orderly cleanup or recovery from program error signals and interactive interrupts.
 * 
 * The cleanest way for a handler to terminate the process is to raise the same signal that ran the handler
 * in the first place. This function uses a static variable to keep track of recursive invocation.
 * 
 * @param sig Signal number passed to the handler function
 * 
 * @source https://www.gnu.org/software/libc/manual/html_node/Termination-in-Handler.html
 */
void handle_termination (int sig)
{

//   Since this handler is established for more than one kind of signal, 
//      it might still get invoked recursively by delivery of some other kind
//      of signal.  Use a static variable to keep track of that. 
  if (fatal_error_in_progress){
    raise (sig);
  } 
  fatal_error_in_progress = 1;

    cleanup_on_exit();

  /* Now reraise the signal.  We reactivate the signal’s
     default handling, which is to terminate the process.
     We could just call exit or abort,
     but reraising the signal sets the return status
     from the process correctly. */
  signal (sig, SIG_DFL);
  raise (sig);
}

/**
 * @brief Get the IP address from a sockaddr structure.
 * 
 * This function returns a pointer to the IP address contained in a sockaddr
 * structure, abstracting away the difference between IPv4 and IPv6.
 * 
 * @param sa Pointer to a sockaddr structure containing an IPv4 or IPv6 address.
 * @return Void pointer to the IP address.
 * 
 * @ref https://beej.us/guide/bgnet/html/#what-is-a-socket Section 6.1
 */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/**
 * @brief Perform global clean-up operations.
 * 
 * This function handles the clean-up tasks for the application, including closing
 * files, deleting temporary data, and closing socket and client descriptors.
 * It also closes the syslog.
 */
void cleanup_on_exit(void)
{
	int ret_status;
	
	// Close data file
	ret_status = close(dataFileDescriptor);
	if(ret_status == ERROR)
	{
		syslog(LOG_ERR,"File close failed");
		s_flags.command_status_success = false;
	}
	
	// delete data file
	ret_status = unlink(DATA_FILE);
	if(ret_status == ERROR)
	{
		syslog(LOG_ERR,"File delete failed");
	}
	
    // Close socket
    if(s_flags.socket_open)
    {
        close(sock_fd);
        s_flags.socket_open = false;
    }
	
    if(s_flags.client_fd_open){
	// close client
	close(clientSocketFd);
    s_flags.client_fd_open = false;
    }

	// Close syslog
	syslog(LOG_INFO,"AESD Socket application end");
	
    //Close log
    if(s_flags.log_open)
    {
        closelog();
        s_flags.log_open = false;
    }
}

int main(int argc, char *argv[])
{
    int opt;

    while((opt = getopt(argc, argv, "d")) != -1)
    {
        if(opt == 'd')
        {
            s_flags.daemon_mode = true;
        }
    }

    main_socket_application();

    return (s_flags.command_status_success) ? 0 : -1;
}

/**
 * @brief Main socket application function
 * 
 * This function sets up a socket server on port 9000 and listens for incoming
 * client connections. It receives data from the client, writes it to a file,
 * reads data from the file, and sends it back to the client. This process
 * continues until the application is terminated.
 * 
 */
void main_socket_application()
{
	// local variables
	int ret_status;
    char s[INET6_ADDRSTRLEN];
	int yes=1; /// For setsockopt() SO_REUSEADDR, below
	
	struct addrinfo hints;
	struct addrinfo *result;
	
	// to create logs from application
	openlog(NULL,0,LOG_USER);
    s_flags.log_open = true;

	syslog(LOG_INFO,"AESD Socket application started");

	if(s_flags.daemon_mode)
		syslog(LOG_INFO,"Started as a deamon");

	DEBUG_LOG("AESD Socket start\n");
	
	// signal handler for SIGINT and SIGTERM
	signal(SIGINT, handle_termination);
	signal(SIGTERM, handle_termination);
    
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;	/* IPv4 */
	hints.ai_socktype = SOCK_STREAM; /* stream socket */
	hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
	hints.ai_protocol = 0;          /* Any protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	
	s_flags.command_status_success = true;
	
	dataFileDescriptor = open(DATA_FILE, O_RDWR | O_CREAT | O_APPEND, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
	if(dataFileDescriptor == ERROR)
	{
        ERROR_LOG("Failed to open file\n");
        syslog(LOG_ERR, "Failed to open file");
		s_flags.command_status_success = false;
	}
	
	do
	{
		if(s_flags.command_status_success == false)
		{
			break;
		}
		ret_status = getaddrinfo(NULL, "9000", &hints, &result);
		if (ret_status != SUCCESS)
		{
            ERROR_LOG("Failure in getaddrinfo()\n");
            syslog(LOG_ERR, "Failure in getaddrinfo()");
			s_flags.command_status_success = false;
			break;
		}
		if(result == NULL)
		{
            ERROR_LOG("Memory allocation failed in getaddrinfo()\n");
            syslog(LOG_ERR, "Memory allocation failed in getaddrinfo()");
			s_flags.command_status_success = false;
			break;
		}

		// creates endpoint for communication
		sock_fd = socket(result->ai_family, result->ai_socktype,
				    result->ai_protocol);
		if(sock_fd == ERROR)
		{
            ERROR_LOG("Failed to create socket\n");
            syslog(LOG_ERR, "Failed to create socket");
			s_flags.command_status_success = false;
			break;
		}
        s_flags.socket_open = true;
		
        
		if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, 
				sizeof(int)) == -1)
               {
            ERROR_LOG("Failed to set socket options\n");
            syslog(LOG_ERR, "Failed to set socket options");
			s_flags.command_status_success = false;
			break;
		}
		
        //bind a address to a socket
		ret_status = bind(sock_fd, result->ai_addr,
			    sizeof(struct sockaddr));
		if(ret_status == ERROR)
		{
            ERROR_LOG("Binding socket operation unsuccessful\n");
            syslog(LOG_ERR, "Binding socket operation unsuccessful");
			s_flags.command_status_success = false;
			break;
		}
		
		// free malloced addr struct returned
		freeaddrinfo(result);
		
        //Check for daemon mode specified by user
		if(s_flags.daemon_mode){
            if(run_daemon() == ERROR){
                break;
            }
        }
		
        //listen for connections on a socket
		ret_status = listen(sock_fd, BACKLOG_CONNECTIONS);
		if(ret_status == ERROR)
		{
            ERROR_LOG("Failed to listen on socket\n");
            syslog(LOG_ERR, "Failed to listen on socket");
			s_flags.command_status_success = false;
			break;
		}
		
		while(1)
		{
            if(accept_and_log_client(s) == ERROR){
                break;
            }
			
			// Receives data over the connection and 
			// appends to file /var/tmp/aesdsocketdata
			ret_status = receive_and_store_data();
			if(s_flags.command_status_success == false)
			{
				break;
			}
			
            //Returns the full content of /var/tmp/aesdsocketdata 
            //to the client as soon as the received data packet 
            //completes.
            ret_status = send_file_content_to_client();
			if(s_flags.command_status_success == false)
			{
				break;
			}

            //Logs message to the syslog “Closed connection from XXX”
            //where XXX is the IP address of the connected client.

            //Restarts accepting connections from new clients forever 
            //in a loop until SIGINT or SIGTERM is received.
			close(clientSocketFd);
            s_flags.client_fd_open = false;
			syslog(LOG_INFO,"Closed connection from %s",s);
		}
		
	}while(0);
	
	cleanup_on_exit();
}


/**
 * @brief Forks the current process to create a daemon.
 * 
 * This function forks the current process to create a daemon by detaching itself
 * from the terminal. It performs necessary setup like changing the file mode,
 * setting a new session, changing the current working directory, and closing
 * standard I/O file descriptors.
 * 
 * @return Returns 0 on success and -1 on failure.
 */
int run_daemon(void)
{
    pid_t forked_pid = fork(); // Fork the current process
    
    // Check for fork failure
    if (forked_pid < 0)
    {
        ERROR_LOG("Failed to fork process\n");
        syslog(LOG_ERR, "Failed to fork process");
        return ERROR;
    }
    
    // If we got a good PID, then we can exit the parent process.
    if (forked_pid > 0)
    {
        syslog(LOG_INFO, "Termination of parent process completed");
        exit(0); // Exit the parent process
    }

    // Unmask the file mode
    umask(0);
    
    // Create a new session ID
    pid_t session_id = setsid();
    
    // Check for session ID failure
    if (session_id < 0)
    {
        exit(1);
    }
    
    // Change the current working directory to root
    chdir("/");
    
    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    return 0;
}

/**
 * @brief Accepts a connection and logs the client's IP.
 *
 * This function listens for and accepts a client connection to the server socket.
 * It logs a message to the syslog containing the IP address of the connected client.
 *
 * @param[out] ip_address A pointer to the character array where the IP address will be stored.
 * @return 0 on success, -1 on failure
 */
int accept_and_log_client(char *ip_address)
{
    struct sockaddr_storage client_addr;
    socklen_t client_addrlen = sizeof(struct sockaddr_storage);

    // Accept a connection
    clientSocketFd = accept(sock_fd, (struct sockaddr *)&client_addr, &client_addrlen);

    if (clientSocketFd == -1)  // Assume ERROR is -1
    {
        syslog(LOG_ERR, "Accept failed");
        s_flags.command_status_success = false;
        return -1;
    }
    s_flags.client_fd_open = true;

    // Get string representation of the client's IP address
    inet_ntop(client_addr.ss_family,
              get_in_addr((struct sockaddr *)&client_addr),
              ip_address, INET6_ADDRSTRLEN);

    syslog(LOG_INFO, "Accepted connection from %s", ip_address);

    return 0;
}


/**
 * @brief Receive data over a connection and append it to a file.
 * 
 * This function receives data from a client connected to the socket and appends it to the file located at /var/tmp/aesdsocketdata.
 *
 * @return 0 on successful operation, -1 otherwise
 */
int receive_and_store_data(void)
{
    ssize_t bytes_received = 1;  // Number of bytes received (initialize to non-zero to start loop)
    char buf[BUF_LEN];  // Buffer to temporarily hold received data
    int ret_status;  // Status returned by the write operation

    // Loop for receiving and storing data
    while (bytes_received > 0)
    {
        // Receive data from the client into the buffer
        bytes_received = recv(clientSocketFd, buf, BUF_LEN, 0);

        // Check for receive errors
        if (bytes_received == ERROR)
        {
            syslog(LOG_ERR, "Receive failed");  
            s_flags.command_status_success = false;
            return -1;
        }

        // Debugging Logs
        DEBUG_LOG("Buffer : %ld", bytes_received);
        DEBUG_LOG("Buffer : %s", buf);

        // Write the received data to the file
        ret_status = write(dataFileDescriptor, buf, bytes_received);

        // Check for write errors
        if (ret_status == ERROR)
        {
            syslog(LOG_ERR, "Writing to file unsuccessful");  
            s_flags.command_status_success = false;
            return -1;
        }

        // Exit condition: Check if the last character in the buffer is a newline
        if (bytes_received > 0 && buf[bytes_received - 1] == '\n')
        {
            break;  // Exit loop if a newline character is encountered
        }
    }

    return 0;  
}

/**
 * @brief Send the content of a file to the client.
 * 
 * This function sends the entire content of the file located at /var/tmp/aesdsocketdata back to the client.
 *
 * @return 0 on successful operation, -1 otherwise
 */
int send_file_content_to_client(void)
{
    ssize_t bytes_transmitted = 0;  // Number of bytes successfully sent to the client
    char file_buffer[BUF_LEN];  // Buffer to hold file content temporarily
    ssize_t read_count = 1;  // Number of bytes read from the file (initialize to non-zero to start loop)

    // Reposition the read/write offset of the file to the beginning
    off_t reposition_status = lseek(dataFileDescriptor, 0, SEEK_SET);
    if (reposition_status == ERROR)
    {
        syslog(LOG_ERR, "lseek operation failed");
        s_flags.command_status_success = false;
        return -1;
    }

    // Loop to handle file reading and sending content
    while (read_count > 0)  // Loop as long as there's content to read
    {
        // Read from the file into the buffer
        read_count = read(dataFileDescriptor, file_buffer, BUF_LEN);
        if (read_count == ERROR)
        {
            syslog(LOG_ERR, "Reading from file failed");  
            s_flags.command_status_success = false;
            return -1;
        }

        // Debugging information
        DEBUG_LOG("Bytes read from file : %ld", read_count);
        DEBUG_LOG("Content in read buffer : \n%s", file_buffer);

        // Transmit the read content to the client
        bytes_transmitted = send(clientSocketFd, file_buffer, read_count, 0);
        if (bytes_transmitted == ERROR)
        {
            syslog(LOG_ERR, "Transmission to client failed");  
            s_flags.command_status_success = false;
            return -1;
        }

        // Exit condition: Check if the last character in the buffer is a newline
        if (read_count > 0 && file_buffer[read_count - 1] == '\n')
        {
            break;  // Exit loop if a newline character is encountered
        }
    }

    return 0;  
}



