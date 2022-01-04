/**      (C)2000-2021 FEUP
 *       tidy up some includes and parameters
 * */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

#define SERVER_CONTROL_PORT 21
#define SERVER_DATA_PORT 20
#define SERVER_ADDR "127.0.0.1"

int main(int argc, char **argv)
{

    if (argc > 1)
        printf("**** No arguments needed. They will be ignored. Carrying ON.\n");
    int sockfd;
    struct sockaddr_in server_addr;
    char buf[255] = "\n";
    size_t bytes;

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

    // TEST CONNECTION

    bytes = send(sockfd, buf, strlen(buf), 0);
    printf("Bytes escritos %ld\n", bytes);

    // RECEIVE TEST

    bytes = recv(sockfd, buf, 255, 0);
    printf("bytes read: %d\n", bytes);
    buf[bytes] = 0;
    printf("%s\n", buf);

    // SEND USER

    const char* auth =  "user nunoa\n"; // \n is needed to denote the end of a command

    bytes = send(sockfd, auth, strlen(auth), 0);
    printf("Bytes escritos %ld\n", bytes);

    bytes = recv(sockfd, buf, 255, 0); // not needed, just for testing
    printf("bytes read: %d\n", bytes);
    buf[bytes] = 0;
    printf("%s\n", buf);

    // SEND PASSWORD

    const char* pass = "PASS <put your system password here>\n";
    bytes = send(sockfd, pass, strlen(pass), 0);
    printf("Bytes escritos %ld\n", bytes);

    bytes = recv(sockfd, buf, 255, 0);
    buf[bytes] = 0;
    printf("%s\n", buf);

    // SEND QUIT

    const char* quit = "QUIT\n";
    bytes = send(sockfd, quit, strlen(quit), 0);
    printf("Bytes escritos %ld\n", bytes);

    bytes = recv(sockfd, buf, 255, 0);
    buf[bytes] = 0;
    printf("%s\n", buf);

    if (close(sockfd) < 0)
    {
        perror("close()");
        exit(-1);
    }
    return 0;
}
