#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "include/constants.h"

int get_socket() {
    struct sockaddr_in server_addr;
    int sockfd;

    /*server address handling*/
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR); /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(SERVER_CONTROL_PORT);            /*server TCP port must be network byte ordered */

    /** FTP CODE MEANINGS
     220 Service ready for new user.
     331 User name okay, need password.
     530 Not logged in.
     221 Service closing control connection.
         Logged out if appropriate.


        TEST USING vsftpd - Very Secure FTP Daemon; you can use your system credentials to test user capabilities with local_enable=YES; on config file see arch wiki
    */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket()");
        exit(-1);
    }
    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connect()");
        exit(-1);
    }
}