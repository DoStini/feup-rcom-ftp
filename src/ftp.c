#include "include/ftp.h"

#include <sys/socket.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>

#include "include/constants.h"
#include "include/socket.h"

typedef struct recv_state recv_state_t;
typedef void recv_state_handler(recv_state_t*, unsigned char);

typedef struct recv_state {
    char code[FTP_CODE_LENGTH];
    uint8_t code_acc;
    recv_state_handler* handler;
} recv_state_t;

recv_state_handler start_recv, multi_line, multi_line_code, read_all, end_recv;

void start_recv(recv_state_t* state, unsigned char ch) {
    if (state->code_acc == FTP_CODE_LENGTH) {
        if (ch == '-') {
            state->handler = multi_line;
        } else if (ch == ' ') {
            state->handler = read_all;
        } else {
            state->code_acc = 0;
        }
    } else {
        if (isdigit(ch)) {
            state->code[state->code_acc] = ch;
            state->code_acc++;
        } else {
            state->code_acc = 0;
        }
    }
}

void multi_line(recv_state_t* state, unsigned char ch) {
    if (ch == '\n') {
        state->handler = multi_line_code;

        state->code_acc = 0;
    }
}

void multi_line_code(recv_state_t* state, unsigned char ch) {
    if (state->code_acc == FTP_CODE_LENGTH) {
        if (ch == ' ') {
            state->handler = read_all;
        } else {
            state->handler = multi_line;
        }
    } else {
        if (isdigit(ch) && state->code[state->code_acc] == ch) {
            state->code_acc++;
        } else {
            state->handler = multi_line;
        }
    }
}

void read_all(recv_state_t* state, unsigned char ch) {
    if (ch == '\n') {
        state->handler = end_recv;
    }
}

void end_recv(recv_state_t* state, unsigned char ch) { return; }

int recv_minimum(int sockfd, char* buffer, size_t buff_size) {
    size_t total = 0;

    while (total <= 0) {
        total = recv(sockfd, buffer, buff_size, 0);
        if (total < 0 && errno != EINTR) {
            return RECV_ERR;
        }
    }

    return total;
}

int send_minimum(int sockfd, char* buffer, size_t buff_size) {
    size_t total = 0;

    while (total <= 0) {
        total = send(sockfd, buffer, buff_size, 0);
        if (total < 0 && errno != EINTR) {
            return SEND_ERR;
        }
    }

    return total;
}

int ftp_recv(int sockfd, char* out_code, char* string, size_t size) {
    char buf[RECV_LENGTH];

    recv_state_t state;
    state.handler = start_recv;
    state.code_acc = 0;
    int total = 0, received;
    bool copy = string != NULL;
    size_t start = 0;

    while (state.handler != end_recv) {
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

        while (start < received && state.handler != end_recv) {
            state.handler(&state, buf[start]);

            start++;
        }
    }

    if (string != NULL) {
        string[total] = 0;
    }
    memcpy(out_code, state.code, FTP_CODE_LENGTH);

    return OK;
}

int ftp_sanity(int sockfd) {
    int err = send_minimum(sockfd, "\n", sizeof "\n" - 1);
    if (err < 0) {
        return err;
    }
    char code[FTP_CODE_LENGTH];
    char message[FTP_MESSAGE_LENGTH];

    err = ftp_recv(sockfd, code, message, FTP_MESSAGE_LENGTH);
    if (err < 0) {
        return err;
    }

    if (strncmp(code, FTP_CODE_READY, FTP_CODE_LENGTH) != 0) {
        fprintf(stderr, "Server not ready:\n%s\n", message);
        return NOT_READY;
    }

    return OK;
}

int ftp_anon_login(int sockfd, url_info_t* info) {
    char* pass = malloc(FTP_MESSAGE_LENGTH * sizeof(char));
    if (pass == NULL) {
        perror("login");
        return LOGIN_ERR;
    }

    printf("Please input your email address (max %d): ", FTP_MESSAGE_LENGTH);
    char* res = fgets(pass, FTP_MESSAGE_LENGTH, stdin);
    if (res == NULL) {
        perror("fgets()");
        return LOGIN_ERR;
    }
    pass[strcspn(pass, "\r\n")] = 0;

    info->password = pass;
    info->user = malloc(sizeof FTP_USR_ANON);
    if (info->user == NULL) {
        perror("login");
        return LOGIN_ERR;
    }
    memcpy(info->user, FTP_USR_ANON, sizeof FTP_USR_ANON);

    return OK;
}

int ftp_login(int sockfd, url_info_t* info) {
    int err = 0;
    char code[FTP_CODE_LENGTH];
    char message[FTP_MESSAGE_LENGTH];

    if (info->user == NULL) {
        err = ftp_anon_login(sockfd, info);
        if (err < 0) {
            return err;
        }
    }

    size_t total = strlen(FTP_CMD_USER) + strlen(info->user) + 2;
    char* user = malloc(total * sizeof(char));
    if (user == NULL) {
        perror("login");
        return LOGIN_ERR;
    }

    err = snprintf(user, total, "%s%s\n", FTP_CMD_USER, info->user);
    if (err < 0) {
        free(user);
        return err;
    }

    err = send_minimum(sockfd, user, total);
    if (err < 0) {
        free(user);
        return err;
    }

    err = ftp_recv(sockfd, code, message, FTP_MESSAGE_LENGTH);
    if (err < 0) {
        free(user);
        return err;
    }

    if (strncmp(code, FTP_CODE_LOGIN, FTP_CODE_LENGTH) != 0) {
        free(user);
        fprintf(stderr, "Couldn't authenticate:\n%s\n", message);
        return LOGIN_ERR;
    }

    free(user);

    return OK;
}
