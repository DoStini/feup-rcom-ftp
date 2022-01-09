#ifndef INCLUDE_SOCKET_H_
#define INCLUDE_SOCKET_H_

#include <stdint.h>

typedef struct url_info {
    char* user;
    char* password;
    char* hostname;
    char* path;
    char* port;
} url_info_t;

int parse_url(const char* server_url, url_info_t* url_information);
void free_url(url_info_t* url_information);
int get_socket(const url_info_t* url_information);
int close_sock(int sockfd);

#endif /* INCLUDE_SOCKET_H_ */
