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
    set_port("ftp", &info);

    int sockfd = get_socket(&info);
    if (sockfd < 0) {
        return sockfd;
    }

    send(sockfd, "\n", strlen("\n"), 0);

    char buf[255];

    int bytes = recv(sockfd, buf, 255, 0);
    buf[bytes] = 0;
    printf("%s\n", buf);

    free_url(&info);
    return close_sock(sockfd);
}
