#ifndef INCLUDE_SOCKET_H_
#define INCLUDE_SOCKET_H_

#include <stdint.h>

int get_socket(const char* server_url);
int close_sock(int sockfd);

#endif /* INCLUDE_SOCKET_H_ */
