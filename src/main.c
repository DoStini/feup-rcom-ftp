#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>

#include "include/socket.h"
#include "include/constants.h"
#include "include/ftp.h"

int main(int argc, char* argv[], char* envp[]) {
    if (argc != 2) {
        fprintf(stderr,
                "main(): Please provide the server URL as an argument\n");
        exit(-1);
    }
    url_info_t info;

    int err = parse_url(argv[1], &info);
    if (err < 0) {
        free_url(&info);
        exit(err);
    }
    err = set_port("ftp", &info);
    if (err < 0) {
        free_url(&info);
        exit(err);
    }

    int sockfd = get_socket(&info);
    if (sockfd < 0) {
        free_url(&info);
        exit(sockfd);
    }

    err = ftp_sanity(sockfd);
    if (err < 0) {
        free_url(&info);
        close_sock(sockfd);
        exit(-1);
    }

    err = ftp_login(sockfd, &info);
    if (err < 0) {
        free_url(&info);
        close_sock(sockfd);
        exit(-1);
    }

    free_url(&info);

    err = ftp_quit(sockfd);
    if (err < 0) {
        close_sock(sockfd);
        exit(-1);
    }

    // int bytes = recv(sockfd, buf, 1000, 0);
    // buf[bytes] = 0;
    // printf("%s <- end read", buf);
    // bytes = recv(sockfd, buf, 1000, 0);
    // buf[bytes] = 0;
    // printf("%s <- end read", buf);
    // //     bytes = recv(sockfd, buf, 1000, 0);
    // // buf[bytes] = 0;
    // // printf("%s\n", buf);

    free_url(&info);
    return close_sock(sockfd);
}
