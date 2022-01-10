#ifndef INCLUDE_FTP_H_
#define INCLUDE_FTP_H_

#include <aio.h>

#define RECV_LENGTH 512
#define FTP_CODE_LENGTH 3
#define FTP_MESSAGE_LENGTH 1024

#define FTP_READY "220"

int ftp_sanity(int sockfd);
int ftp_recv(int sockfd, char* out_code, char* str, size_t size);

#endif /* INCLUDE_FTP_H_  */
