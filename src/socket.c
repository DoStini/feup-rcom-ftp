#include "include/socket.h"

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>

#include "include/constants.h"

int parse_url(const char* server_url, url_info_t* url_information) {
    regex_t preg;
    const char* pattern =
        "(ftp://)(([^/@:]+):([^/@:]*)@([^/@:]+)|([^/@:]*)@([^/@:]+)|([^/@:]+))/"
        "(.+)";
    char error_message[200];

    int err = regcomp(&preg, pattern, REG_EXTENDED);
    if (err != 0) {
        regerror(err, &preg, error_message, sizeof(error_message));
        fprintf(stderr, "%s\n", error_message);

        regfree(&preg);
        return ADDR_REGEX_ERR;
    }

    regmatch_t matches[10];
    err = regexec(&preg, server_url, 10, matches, 0);
    if (err != 0) {
        fprintf(stderr,
                "The given URL is not valid, it must follow the following "
                "format:\nftp://[<user>[:[<password>]]@]<host>/<url-path>\n"
                "Make sure the user, password and host don't include '@', '/' "
                "or ':'\n");

        regfree(&preg);
        return ADDR_REGEX_ERR;
    }

    printf("%d\n", matches->rm_so);

    // char test[1000];
    // memcpy(test, &server_url[matches[9].rm_so], matches[9].rm_eo);
    // test[matches[9].rm_eo] = '\0';
    // printf("%s\n", test);

    regfree(&preg);
    return OK;
}

struct addrinfo* get_addrinfo(const url_info_t* url_information) {
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;  // IPV4
    hints.ai_socktype =
        SOCK_STREAM;  // STREAM SOCKETS - PACKETS ARRIVE IN ORDER
    hints.ai_protocol = IPPROTO_TCP;  // USING TCP PROTOCOL

    struct addrinfo* results;
    int err;

    if ((err = getaddrinfo(url_information->hostname, url_information->port,
                           &hints, &results)) != 0) {
        herror("getaddrinfo()");
        return NULL;
    }

    return results;
}

int get_socket(const url_info_t* url_information) {
    int sockfd;

    struct addrinfo *results = get_addrinfo(url_information), *it;
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
