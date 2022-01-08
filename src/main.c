#include <stdio.h>

#include "include/socket.h"
#include "include/constants.h"

int main(int argc, char* argv[], char* envp[]) {
    int sockfd = get_socket(SERVER_ADDR, SERVER_CONTROL_PORT);
    if(sockfd < 0) {
        return sockfd;
    }
    
    return close_sock(sockfd);
}
