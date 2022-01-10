#ifndef INCLUDE_FTP_H_
#define INCLUDE_FTP_H_

#include <aio.h>

int ftp_recv(int sockfd, char* out_code, char* str, size_t size);

#endif /* INCLUDE_FTP_H_  */
