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

int send_all(int sockfd, char* buffer, size_t buff_size) {
    int err;
    size_t total = 0;

    while (total < buff_size) {
        err = send(sockfd, buffer + total, buff_size - total, 0);
        if (err < 0 && errno != EINTR) {
            return SEND_ERR;
        }

        total += err;
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
    int err = send_all(sockfd, "\n", sizeof "\n" - 1);
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

int get_pass(const char* message, url_info_t* info) {
    info->password = malloc(FTP_MESSAGE_LENGTH * sizeof(char));
    if (info->password == NULL) {
        perror("login");
        return LOGIN_ERR;
    }

    printf("%s (max %d): ", message, FTP_MESSAGE_LENGTH);
    char* res = fgets(info->password, FTP_MESSAGE_LENGTH, stdin);
    if (res == NULL) {
        free(info->password);
        info->password = NULL;
        perror("fgets()");
        return LOGIN_ERR;
    }
    info->password[strcspn(info->password, "\r\n")] = 0;

    return OK;
}

int ftp_anon_login(int sockfd, url_info_t* info) {
    int err = get_pass("Please input your email address", info);
    if (err < 0) {
        return err;
    }

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

    err = send_all(sockfd, user, total-1);
    if (err < 0) {
        free(user);
        return err;
    }

    err = ftp_recv(sockfd, code, message, FTP_MESSAGE_LENGTH);
    if (err < 0) {
        free(user);
        return err;
    }

    free(user);

    if (strncmp(code, FTP_CODE_LOGIN, FTP_CODE_LENGTH) == 0) {
        printf("Success Logging in!\n");
        return OK;
    }

    if (strncmp(code, FTP_CODE_PASS, FTP_CODE_LENGTH) == 0) {
        if (info->password == NULL) {
            int err =
                get_pass("Password Required, please input your password", info);
            if (err < 0) {
                return err;
            }
        }

        total = strlen(FTP_CMD_PASS) + strlen(info->password) + 2;
        char* auth = malloc(total * sizeof(char));
        if (auth == NULL) {
            perror("login");
            return LOGIN_ERR;
        }

        err = snprintf(auth, total, "%s%s\n", FTP_CMD_PASS, info->password);
        if (err < 0) {
            free(auth);
            return err;
        }

        err = send_all(sockfd, auth, total-1);
        if (err < 0) {
            free(auth);
            return err;
        }

        err = ftp_recv(sockfd, code, message, FTP_MESSAGE_LENGTH);
        if (err < 0) {
            free(auth);
            return err;
        }

        free(auth);

        if (strncmp(code, FTP_CODE_LOGIN, FTP_CODE_LENGTH) == 0) {
            printf("Success Logging in!\n");
            return OK;
        } else {
            fprintf(stderr, "Couldn't authenticate:\n%s\n", message);
            return LOGIN_ERR;
        }
    } else {
        fprintf(stderr, "Couldn't authenticate:\n%s\n", message);
        return LOGIN_ERR;
    }

    return OK;
}
