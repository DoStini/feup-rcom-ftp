#include "include/socket.h"

#include <stdio.h>
#include <sys/cdefs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>

#include "include/constants.h"
#include "include/regex.h"

//  match 0 -> full match
//      match 1 -> credentials: -> if null then user is "anonymous"
//          match 2 -> user name with : (: not included in matched string)
//          match 3 -> password which can be empty
//          --------- OR ---------
//          match 4 -> user name without :
//      match 5 -> host name
//      match 6 -> file path
//
int parse_regmatch(const char* server_url, const regmatch_t* matches,
                   url_info_t* url_information) {
    int err;

    if (matches[2].rm_so != -1) {
        err =
            regmatch_to_string(server_url, matches[2], &url_information->user);
        if (err < 0) {
            return err;
        }
        err = regmatch_to_string(server_url, matches[3],
                                 &url_information->password);
        if (err < 0) {
            return err;
        }
    } else if (matches[4].rm_so != -1) {
        err =
            regmatch_to_string(server_url, matches[4], &url_information->user);
        if (err < 0) {
            return err;
        }
    }

    err =
        regmatch_to_string(server_url, matches[5], &url_information->hostname);
    if (err < 0) {
        return err;
    }
    err = regmatch_to_string(server_url, matches[6], &url_information->path);
    if (err < 0) {
        return err;
    }

    return OK;
}

int parse_url(const char* server_url, url_info_t* url_information) {
    regex_t preg;
    const char* pattern =
        "ftp://(([^/@:]+):([^/@:]*)@|([^/@:]*)@|)([^/@:]+)/(.+)";
    char error_message[200];
    memset(url_information, 0, sizeof(url_info_t));

    int err = regcomp(&preg, pattern, REG_EXTENDED);
    if (err != 0) {
        regerror(err, &preg, error_message, sizeof(error_message));
        fprintf(stderr, "%s\n", error_message);

        regfree(&preg);
        return ADDR_REGEX_ERR;
    }

    regmatch_t matches[7];
    err = regexec(&preg, server_url, 7, matches, 0);
    if (err != 0) {
        fprintf(stderr,
                "The given URL is not valid, it must follow the following "
                "format:\nftp://[<user>[:<password>]@]<host>/<url-path>\n"
                "Make sure the user, password and host don't include '@', '/' "
                "or ':'\n");

        regfree(&preg);
        return ADDR_REGEX_ERR;
    }

    regfree(&preg);
    return parse_regmatch(server_url, matches, url_information);
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
    int sockfd = SOCKET_ERR;

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
            close_sock(sockfd);
            perror("connect()");
            continue;
        }

        break;
    }

    freeaddrinfo(results);

    if (it == NULL) {
        fprintf(stderr, "failed to connect\n");

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

int set_port(const char* port, url_info_t* url_information) {
    size_t size = strlen(port) + 1;
    url_information->port = malloc(size * sizeof(char));
    if (url_information->port == NULL) {
        return MEMORY_ERR;
    }

    memcpy(url_information->port, port, size);
    return OK;
}

int get_pass(const char* message, url_info_t* info) {
    info->password = malloc(MESSAGE_LENGTH * sizeof(char));
    if (info->password == NULL) {
        perror("login");
        return LOGIN_ERR;
    }

    printf("%s (max %d): ", message, MESSAGE_LENGTH);
    char* res = fgets(info->password, MESSAGE_LENGTH, stdin);
    if (res == NULL) {
        free(info->password);
        info->password = NULL;
        perror("fgets()");
        return LOGIN_ERR;
    }
    info->password[strcspn(info->password, "\r\n")] = 0;

    return OK;
}

void free_url(url_info_t* url_information) {
    if (url_information->hostname != NULL) {
        free(url_information->hostname);
        url_information->hostname = NULL;
    }
    if (url_information->password != NULL) {
        free(url_information->password);
        url_information->password = NULL;
    }
    if (url_information->path != NULL) {
        free(url_information->path);
        url_information->path = NULL;
    }
    if (url_information->port != NULL) {
        free(url_information->port);
        url_information->port = NULL;
    }
    if (url_information->user != NULL) {
        free(url_information->user);
        url_information->user = NULL;
    }
}

int open_data_connection(uint32_t address, uint16_t port) {
    int sockfd;
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(address);
    server_addr.sin_port = htons(port);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return SOCKET_ERR;
    }

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) <
        0) {
        perror("connect()");
        return SOCKET_ERR;
    }

    return sockfd;
}
