#include "aesdsocket.h"

// socket() creates an endpoint for communication and returns a file
//        descriptor that refers to that endpoint.  The file descriptor
//        returned by a successful call will be the lowest-numbered file
//        descriptor not currently open for the process.

//sock socket(int domain, int type, int protocol);

int main(void){

    int status;
    struct addrinfo hints, *servinfo;  // will point to the results
    struct sockaddr_storage client_addr; //Structure to hold socket addresses
    socklen_t client_addrlen;
    char ipstr[INET6_ADDRSTRLEN];
    FILE *fp;


    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    if ((status = getaddrinfo(NULL, "9000", &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // ... do everything until you don't need servinfo anymore ....

    // Initialize syslog
    openlog("MySocketProgram", LOG_PID | LOG_CONS, LOG_USER);

    int sockfd = socket(AF_UNSPEC, SOCK_STREAM, 0);
    if (sockfd == -1) {
        syslog(LOG_ERR, "Socket creation failed. Errno: %d", errno);
        closelog(); // Close the log
        return 1;
    }

    syslog(LOG_INFO, "Socket created successfully. File descriptor: %d", sockfd);
    closelog(); // Close the log

    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        syslog(LOG_ERR, "Bind failed. Errno: %d", errno);
        closelog();
        return -1;
    }

    if (listen(sockfd, 5) == -1) {
        syslog(LOG_ERR, "Listen failed. Errno: %d", errno);
        closelog();
        return -1;
    }
    //freeaddrinfo (free since addrinfo is malloced)
    
    freeaddrinfo(servinfo); // free the linked-list

    client_addrlen = sizeof client_addr;
    int client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addrlen);

    if (client_fd == -1) {
        syslog(LOG_ERR, "Accept failed. Errno: %d", errno);
        closelog();
        return -1;
    }

    inet_ntop(client_addr.ss_family, &((struct sockaddr_in*)&client_addr)->sin_addr, ipstr, sizeof ipstr);
    syslog(LOG_INFO, "Accepted connection from %s", ipstr);

    char buf[1024];
    ssize_t bytes_received;
    fp = fopen("/var/tmp/aesdsocketdata", "a");

    if (fp == NULL) {
        syslog(LOG_ERR, "Failed to open file. Errno: %d", errno);
        closelog();
        return -1;
    }

    while ((bytes_received = recv(client_fd, buf, sizeof(buf)-1, 0)) > 0) {
        buf[bytes_received] = '\0';
        fwrite(buf, sizeof(char), bytes_received, fp);
    }

    fclose(fp);
    freeaddrinfo(servinfo);
    closelog();

    return 0;

}