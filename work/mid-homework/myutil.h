#ifndef MY_UTIL_H
#define MY_UTIL_H

#include "lib/buffer.h"

#define BUF_SIZE 4096

void rand_str(char *dest, int length);
void trim_space(char *orig);
void ls(char *buf, int len);
int tcp_send(int fd, char *buf, int len);
void message_process(int connfd, char *msg, int len);
struct buffer* message_process1(struct buffer* input);

#endif