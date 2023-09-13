
/*
 *  
 *  Author     : Suraj Ajjampur
 *  email      : suraj.ajjampur@colorado.edu 
 *  Course     : AESD - ECEN5713
 *  Assignment : 02
 * 
 */

/***************************************** HEADER_FILES *******************************************************/
#include <syslog.h>
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>

/***************************************** Main Function *******************************************************/
int main(int argc, char* argv[]){
  if (argc!=3){                                                                  /* Checking for valid no. of args */
    openlog(NULL, 0, LOG_USER);                                                  /* Open Log File */ 
    syslog(LOG_ERR, "Invalid Number of Arguments: %d\n", argc);                  /* Error Logging */
    closelog();                                                                  
    return 1;
  }
  
  int fp = open(argv[1], O_CREAT | O_WRONLY , S_IRWXU | S_IRWXG | S_IRWXO);      /* Open the file to write */
  if (fp != -1){                                                                 /* Check validity of the file pointer */
    int i = strlen(argv[2]);                                                     /* Check for total chars written into the file */
    int wr_status = write(fp, argv[2], i);                                       /* Write into file */
    openlog(NULL, 0, LOG_USER);                                                  
    if (wr_status==-1){                                                          /* Check for successful write action */
      syslog(LOG_ERR, "Write Operation failed with return -1\n\r");
      closelog();
      close(fp);                                                                 /* Close file descriptor */
      return 1;
    }
    else if (wr_status < i){                                                     /* Check if partial no. of bytes written */
      syslog(LOG_ERR, "Not all bytes are written to the file W - \n\r");
      closelog();
      close(fp);                                                                 /* Close file descriptor */
      return 1;
    }
    else{									 /* Case of write operation successful */ 

      syslog(LOG_DEBUG, "Write Operation is Successful \n\r");
      closelog();
      close(fp);                                                                 /* Close file descriptor */

    }

  }
  else{

    openlog(NULL, 0, LOG_USER);                                                  /* Open Log file */
    syslog(LOG_ERR, "File Descriptor failed with return -1\n\r");                /* Log error message */
    closelog();									 /* Close file descriptor */

    return 1;
  }

  return 0;
}
