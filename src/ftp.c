#include "include/ftp.h"

#include <sys/socket.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "include/socket.h"
#include "include/constants.h"

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
    int err = send_minimum(sockfd, "\n", sizeof "\n");
    if (err < 0) {
        return err;
    }
    char code[FTP_CODE_LENGTH];
    char message[FTP_MESSAGE_LENGTH];

    err = ftp_recv(sockfd, code, message, FTP_MESSAGE_LENGTH);
    if (err < 0) {
        return err;
    }

    if (strncmp(code, FTP_READY, FTP_CODE_LENGTH) != 0) {
        fprintf(stderr, "Server not ready:\n%s\n", message);
        return NOT_READY;
    }

    return OK;
}

int ftp_login(int sockfd, url_info_t* info) {}
