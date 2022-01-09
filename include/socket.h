#ifndef INCLUDE_SOCKET_H_
#define INCLUDE_SOCKET_H_

#include <stdint.h>

typedef struct url_info {
    const char* user;
    const char* password;
    const char* hostname;
    const char* path;
    const char* port; 
} url_info_t;

int parse_url(const char* server_url, url_info_t* url_information);
int get_socket(const url_info_t* url_information);
int close_sock(int sockfd);

#endif /* INCLUDE_SOCKET_H_ */
