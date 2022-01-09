#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "include/constants.h"

struct addrinfo* get_addrinfo(const char* server_url) {
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;  // IPV4
    hints.ai_socktype =
        SOCK_STREAM;  // STREAM SOCKETS - PACKETS ARRIVE IN ORDER
    hints.ai_protocol = IPPROTO_TCP;  // USING TCP PROTOCOL

    struct addrinfo* results;
    int err;

    if ((err = getaddrinfo(server_url, "ftp", &hints, &results)) != 0) {
        herror("getaddrinfo()");
        return NULL;
    }

    return results;
}

int get_socket(const char* server_url) {
    int sockfd;

    struct addrinfo *results = get_addrinfo(server_url), *it;
    if (results == NULL) {
        return SOCKET_ERR;
    }

    for (it = results; it != NULL; it = it->ai_next) {
        // USING STREAM SOCKETS TO ENSURE DATA ARRIVES IN ORDER
        if ((sockfd = socket(it->ai_family, it->ai_socktype, 0)) < 0) {
            perror("socket()");
            continue;
        }

        if (connect(sockfd, it->ai_addr, it->ai_addrlen) < 0) {
            perror("connect()");
            continue;
        }

        break;
    }

    freeaddrinfo(results);

    if (it == NULL) {
        fprintf(stderr, "failed to connect");
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
