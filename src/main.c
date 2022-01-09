#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>

#include "include/socket.h"
#include "include/constants.h"

int main(int argc, char* argv[], char* envp[]) {
    if (argc != 2) {
        fprintf(stderr,
                "main(): Please provide the server URL as an argument\n");
        exit(-1);
    }
    url_info_t info;

    parse_url(argv[1], &info);

    printf("user: %s\nhostname: %s\npath: %s\n", info.user, info.hostname,
           info.path);
    if (info.password != NULL) {
        printf("has pass: %s\n", info.password);
    } else {
        printf("no pass given\n");
    }

    free_url(&info);

    // int sockfd = get_socket(SERVER_ADDR);
    // if (sockfd < 0) {
    //     return sockfd;
    // }

    // send(sockfd, "\n", strlen("\n"), 0);

    // char buf[255];

    // int bytes = recv(sockfd, buf, 255, 0);
    // buf[bytes] = 0;
    // printf("%s\n", buf);

    // return close_sock(sockfd);
}
