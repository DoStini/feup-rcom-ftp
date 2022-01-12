#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>

#include "include/socket.h"
#include "include/constants.h"
#include "include/ftp.h"

int main(int argc, char* argv[], char* envp[]) {
    const char* folder;
    if (argc == 3) {
        folder = argv[2];
    } else if (argc == 2) {
        folder = "data/";
    } else {
        fprintf(stderr,
                "main(): Please provide the server URL as an argument\n");
        exit(-1);
    }
    printf("datapath %s\n\n\n", folder);

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

    int data_sockfd = ftp_passive(sockfd);
    if (data_sockfd < 0) {
        free_url(&info);
        close_sock(sockfd);
        exit(-1);
    }

    err = ftp_retr(sockfd, data_sockfd, &info, folder);

    free_url(&info);
    close_sock(data_sockfd);

    if (err < 0) {
        close_sock(sockfd);
        exit(-1);
    }

    err = ftp_quit(sockfd);
    if (err < 0) {
        close_sock(sockfd);
        exit(-1);
    }

    return close_sock(sockfd);
}
