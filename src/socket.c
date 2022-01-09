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

int regmatch_to_string(const char* server_url, const regmatch_t match,
                       char** string) {
    size_t size = match.rm_eo - match.rm_so;
    *string = malloc((size + 1) * sizeof(char));
    if ((*string) == NULL) {
        return MEMORY_ERR;
    }

    memcpy(*string, &server_url[match.rm_so], size);
    (*string)[size] = '\0';

    return OK;
}

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
    memset(url_information, 0, sizeof(url_info_t));
    size_t first_size = matches[1].rm_eo - matches[1].rm_so;
    int err;

    if (first_size == 0) {
        size_t anon_length = sizeof "anonymous";
        url_information->user = malloc(anon_length * sizeof(char));
        if (url_information->user == NULL) {
            return MEMORY_ERR;
        }

        memcpy(url_information->user, "anonymous", anon_length);
    } else {
        if (matches[2].rm_so != -1) {
            err = regmatch_to_string(server_url, matches[2],
                                     &url_information->user);
            if (err < 0) {
                return err;
            }
            err = regmatch_to_string(server_url, matches[3],
                                     &url_information->password);
            if (err < 0) {
                return err;
            }
        } else {
            err = regmatch_to_string(server_url, matches[4],
                                     &url_information->user);
            if (err < 0) {
                return err;
            }
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
                "format:\nftp://[<user>[:[<password>]]@]<host>/<url-path>\n"
                "Make sure the user, password and host don't include '@', '/' "
                "or ':'\n");

        regfree(&preg);
        return ADDR_REGEX_ERR;
    }

    regfree(&preg);
    return parse_regmatch(server_url, matches, url_information);
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
            close_sock(sockfd);
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
        fprintf(stderr, "failed to connect");

        close_sock(sockfd);
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
