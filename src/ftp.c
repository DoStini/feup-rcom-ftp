#include "include/ftp.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <regex.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

#include "include/constants.h"
#include "include/regex.h"
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
    char buf;

    recv_state_t state;
    state.handler = start_recv;
    state.code_acc = 0;
    int total = 0, received;
    bool copy = string != NULL;

    while (state.handler != end_recv) {
        received = recv_minimum(sockfd, &buf, 1);
        if (received < 0) {
            return received;
        }

        if (copy) {
            if (total + received > size) {
                copy = false;
                total = size;
            } else {
                string[total] = buf;
                total += received;
            }
        }

        state.handler(&state, buf);
    }

    if (string != NULL) {
        string[total] = 0;
    }
    memcpy(out_code, state.code, FTP_CODE_LENGTH);

    return OK;
}

int ftp_send(int sockfd, char* ftp_code, char* param) {
    size_t total = strlen(ftp_code) + strlen(param) + 3;
    char* cmd = malloc(total * sizeof(char));
    if (cmd == NULL) {
        perror("login");
        return LOGIN_ERR;
    }

    int err = snprintf(cmd, total, "%s%s\r\n", ftp_code, param);
    if (err < 0) {
        free(cmd);
        return err;
    }

    err = send_all(sockfd, cmd, total - 1);
    if (err < 0) {
        free(cmd);
        return err;
    }

    free(cmd);
    return OK;
}

int ftp_sanity(int sockfd) {
    char code[FTP_CODE_LENGTH];
    char message[FTP_MESSAGE_LENGTH];

    int err = ftp_recv(sockfd, code, message, FTP_MESSAGE_LENGTH);
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

    err = ftp_send(sockfd, FTP_CMD_USER, info->user);
    if (err < 0) {
        return err;
    }

    err = ftp_recv(sockfd, code, message, FTP_MESSAGE_LENGTH);
    if (err < 0) {
        return err;
    }

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

        err = ftp_send(sockfd, FTP_CMD_PASS, info->password);
        if (err < 0) {
            return err;
        }

        err = ftp_recv(sockfd, code, message, FTP_MESSAGE_LENGTH);
        if (err < 0) {
            return err;
        }

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

int ftp_quit(int sockfd) {
    char code[FTP_CODE_LENGTH];
    char message[FTP_MESSAGE_LENGTH];

    int err = ftp_send(sockfd, FTP_CMD_QUIT, "");
    if (err < 0) {
        return err;
    }

    err = ftp_recv(sockfd, code, message, FTP_MESSAGE_LENGTH);
    if (err < 0) {
        return err;
    }

    if (strncmp(code, FTP_CODE_QUIT, FTP_CODE_LENGTH) == 0) {
        printf("Logged out.\n");
        return OK;
    } else {
        fprintf(stderr, "Couldn't log out:\n%s\n", message);
        return LOGIN_ERR;
    }
}

int parse_regmatch_string(const char* original, const regmatch_t match,
                          char** result) {
    int err;

    if (match.rm_so != -1) {
        err = regmatch_to_string(original, match, result);
        if (err < 0) {
            return err;
        }
    }
    return OK;
}

int ftp_passive(int sockfd) {
    int err = 0;
    char code[FTP_CODE_LENGTH];
    char message[FTP_MESSAGE_LENGTH];

    err = ftp_send(sockfd, FTP_CMD_PASV, "");
    if (err < 0) {
        return err;
    }

    err = ftp_recv(sockfd, code, message, FTP_MESSAGE_LENGTH);
    if (err < 0) {
        return err;
    }

    if (strncmp(code, FTP_CODE_PASV, FTP_CODE_LENGTH) != 0) {
        fprintf(stderr, "Couldn't get address and port:\n%s\n", message);
        return PASV_ERR;
    }

    regex_t preg;

    const char* pattern =
        "([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3}),([0-"
        "9]{1,3})";

    char error_message[200];

    err = regcomp(&preg, pattern, REG_EXTENDED);
    if (err != 0) {
        regerror(err, &preg, error_message, sizeof(error_message));
        fprintf(stderr, "%s\n", error_message);

        regfree(&preg);
        return ADDR_REGEX_ERR;
    }

    regmatch_t matches[7];
    err = regexec(&preg, message, 7, matches, 0);
    if (err != 0) {
        fprintf(stderr, "Passive mode return message contains an error");

        regfree(&preg);
        return ADDR_REGEX_ERR;
    }

    uint32_t address = 0;
    uint32_t mults[4] = {16777216, 65536, 256, 1};

    for (uint32_t i = 0; i < 4; i++) {
        char* temp;
        parse_regmatch_string(message, matches[i + 1], &temp);

        int match = atoi(temp);
        address += match * mults[i];

        free(temp);
    }

    uint16_t port = 0;

    for (uint16_t i = 0; i < 2; i++) {
        char* temp;
        parse_regmatch_string(message, matches[i + 5], &temp);

        int match = atoi(temp);
        port += match * mults[i + 2];

        free(temp);
    }

    regfree(&preg);

    return open_data_connection(address, port);
}

int ftp_read_file(int sockfd, int data_sockfd, const char* filename) {
    char buffer[RECV_LENGTH];
    int bytes, err;

    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        perror("fopen()");
        return RETR_ERR;
    }

    while ((bytes = recv(data_sockfd, buffer, RECV_LENGTH, 0))) {
        err = fwrite(buffer, 1, bytes, file);
        if (err < 0) {
            perror("fwrite()");
            return RETR_ERR;
        }

        // printf("recv %d err %d\n", bytes, err);
        // for (int i = 0; i < err; i++) {
        // printf("%x", buffer[i]);
        // }
        // printf("\nnew line\n");
    }

    return fclose(file);
}

int ftp_retr(int sockfd, int data_sockfd, url_info_t* info,
             const char* folder) {
    int err = 0;
    char code[FTP_CODE_LENGTH];
    char message[FTP_MESSAGE_LENGTH];

    err = ftp_send(sockfd, FTP_CMD_RETR, info->path);
    if (err < 0) {
        return err;
    }

    err = ftp_recv(sockfd, code, message, FTP_MESSAGE_LENGTH);
    if (err < 0) {
        return err;
    }

    if (strncmp(code, FTP_CODE_FILE_BEGIN, FTP_CODE_LENGTH) != 0 &&
        strncmp(code, FTP_CODE_FILE_START, FTP_CODE_LENGTH) != 0) {
        printf("Couldn't transfer file:\n%s\n", message);
        return RETR_ERR;
    }

    char* ptr = strtok(info->path, "/");
    char* prev = "";

    while (ptr != NULL) {
        prev = ptr;
        ptr = strtok(NULL, "/");
    }

    size_t path_size = strlen(folder) + strlen(prev) + 1;
    char* copy = malloc(path_size);
    snprintf(copy, path_size, "%s%s", folder, prev);

    err = ftp_read_file(sockfd, data_sockfd, copy);
    free(copy);

    if (err < 0) {
        return err;
    }

    err = ftp_recv(sockfd, code, message, FTP_MESSAGE_LENGTH);
    if (err < 0) {
        return err;
    }

    if (strncmp(code, FTP_CODE_COMPLETE, FTP_CODE_LENGTH) != 0) {
        printf("Couldn't transfer file:\n%s\n", message);
        return RETR_ERR;
    }

    return OK;
}
