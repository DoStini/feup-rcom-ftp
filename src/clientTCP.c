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

#define SERVER_PORT 21
#define SERVER_ADDR "127.0.0.1"

int main(int argc, char **argv)
{

    if (argc > 1)
        printf("**** No arguments needed. They will be ignored. Carrying ON.\n");
    int sockfd;
    struct sockaddr_in server_addr;
    char buf[255] = "";
    size_t bytes;

    /*server address handling*/
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR); /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(SERVER_PORT);            /*server TCP port must be network byte ordered */

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
    /*send a string to the server*/
    bytes = send(sockfd, buf, strlen(buf), 0);
    printf("Bytes escritos %ld\n", bytes);

    bytes = recv(sockfd, buf, 255, 0);
    printf("bytes read: %d\n", bytes);
    buf[bytes] = 0;

    printf("%s\n", buf);

    const char* auth =  "user nunoa";

    bytes = send(sockfd, auth, strlen(auth), 0);
    printf("Bytes escritos %ld\n", bytes);

    bytes = recv(sockfd, buf, 255, 0);
    printf("bytes read: %d\n", bytes);
    buf[bytes] = 0;

    printf("%s\n", buf);

    const char* pass = "PASS C";

    bytes = send(sockfd, pass, strlen(pass), 0);

    if (bytes > 0)
    printf("Bytes escritos %ld\n", bytes);
    else
    {
        perror("write()");
        exit(-1);
    }

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
