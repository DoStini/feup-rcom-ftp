#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "include/constants.h"

int get_socket(const char* server_url, uint16_t port) {
    struct sockaddr_in server_addr;
    int sockfd;

    /*server address handling*/
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    /*32 bit Internet address network byte ordered*/
    server_addr.sin_addr.s_addr = inet_addr(server_url);
    /*server TCP port must be network byte ordered */
    server_addr.sin_port = htons(port);

    /*open a TCP socket*/
    // USING STREAM SOCKETS TO ENSURE DATA ARRIVES IN ORDER
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return SOCKET_ERR;
    }

    /*connect to the server*/
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) <
        0) {
        perror("connect()");
        return SOCKET_ERR;
    }

    return sockfd;
}

int close_sock(int sockfd) {
    if (close(sockfd) < 0) {
        perror("close()");
        return SOCKET_ERR;
    }

    return OK;
}
