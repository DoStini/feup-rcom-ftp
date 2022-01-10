#ifndef INCLUDE_FTP_H_
#define INCLUDE_FTP_H_

#include <aio.h>
#include "include/socket.h"

#define RECV_LENGTH 512
#define FTP_CODE_LENGTH 3
#define FTP_MESSAGE_LENGTH 1024

#define FTP_CODE_READY "220"
#define FTP_CODE_LOGIN "230"
#define FTP_CODE_PASS "331"
#define FTP_CODE_LOGIN_FAIL "530"

#define FTP_CMD_USER "USER "
#define FTP_CMD_PASS "PASS "

#define FTP_USR_ANON "anonymous"

int ftp_sanity(int sockfd);
int ftp_recv(int sockfd, char* out_code, char* str, size_t size);
int ftp_login(int sockfd, url_info_t* info);

#endif /* INCLUDE_FTP_H_  */
