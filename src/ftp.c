#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "include/socket.h"
#include "include/constants.h"

typedef enum recv_state {
    START_RECV,
    MULTI_LINE,
    MULTI_LINE_CODE,
    READ_ALL,
    END_RECV
} recv_state_t;

int recv_minimum(int sockfd, char* buffer, size_t buff_size) {
    int received;
    size_t total = 0;

    while (total == 0) {
        received = recv(sockfd, buffer + total, buff_size - total, 0);
        if (received < 0 && errno != EINTR) {
            return RECV_ERR;
        } else {
            total += received;
        }
    }

    return total;
}

int ftp_recv(int sockfd, char* out_code, char* string, size_t size) {
    char buf[RECV_LENGTH];
    char code[FTP_CODE_LENGTH];
    char end_code[FTP_CODE_LENGTH];

    recv_state_t state = START_RECV;
    int total = 0, received;
    bool copy = string != NULL;
    size_t start = 0;
    size_t code_acc = 0;

    while (state != END_RECV) {
        start = 0;
        received = recv_minimum(sockfd, buf, RECV_LENGTH);
        if (received < 0) {
            return received;
        }

        if (copy) {
            if (total + received > size) {
                memcpy(string + total, buf, size - total);
                copy = false;

                total = size;
            } else {
                memcpy(string + total, buf, received);

                total += received;
            }
        }

        while (start < received && state != END_RECV) {
            switch (state) {
                case START_RECV:
                    if (code_acc == FTP_CODE_LENGTH) {
                        if (buf[start] == '-') {
                            state = MULTI_LINE;
                        } else if (buf[start] == ' ') {
                            state = READ_ALL;
                        } else {
                            code_acc = 0;
                        }
                    } else {
                        if (isdigit(buf[start])) {
                            code[code_acc] = buf[start];
                            code_acc++;
                        } else {
                            code_acc = 0;
                        }
                    }

                    break;
                case MULTI_LINE:
                    if (buf[start] == '\n') {
                        state = MULTI_LINE_CODE;

                        code_acc = 0;
                    }

                    break;
                case MULTI_LINE_CODE:
                    if (code_acc == FTP_CODE_LENGTH) {
                        if (buf[start] == ' ' &&
                            strncmp(code, end_code, FTP_CODE_LENGTH) == 0) {
                            state = READ_ALL;
                        } else {
                            state = MULTI_LINE;
                        }
                    } else {
                        if (isdigit(buf[start])) {
                            end_code[code_acc] = buf[start];
                            code_acc++;
                        } else {
                            state = MULTI_LINE;
                        }
                    }

                    break;
                case READ_ALL:
                default:
                    if (buf[start] == '\n') {
                        state = END_RECV;
                    }

                    break;
                case END_RECV:
                    break;
            }

            start++;
        }
    }

    if (string != NULL) {
        string[total] = 0;
    }
    memcpy(out_code, code, FTP_CODE_LENGTH);

    return OK;
}

int ftp_login(int sockfd, url_info_t* info) {}
