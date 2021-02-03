#include "lib/common.h"

#define BUFSIZE 4096

static int running = 1;

void sig_handler(int signo) {
    running = 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        error(1, 0, "usage: client <IP> <port>\n");
    }

    if (SIG_ERR == signal(SIGINT, sig_handler)) {
        error(1, errno, "signal error");
    }
    int port = atoi(argv[2]);
    int fd = tcp_client(argv[1], port);
    char sendbuf[4096];
    char recvbuf[4096];

    fd_set readmask;
    fd_set allreads;
    FD_ZERO(&allreads);
    FD_SET(fd, &allreads);
    FD_SET(STDIN_FILENO, &allreads);

    while (running) {
        readmask = allreads;
        printf("input your command('quit' to exit):\n");
        int n = select(fd + 1, &readmask, NULL, NULL, NULL);
        if (n <= 0) {
            error(0, errno, "select error");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &readmask)) {
            if (!fgets(sendbuf, sizeof(sendbuf), stdin)) {
                error(0, errno, "fgets error");
                if (errno == EINTR) {
                    continue;
                } else {
                    break;
                }
            }
            int len = strlen(sendbuf);
            //printf("string: %s, len = %d\n", sendbuf, len);
            //TODO: 换行的处理存在一定问题
            if (sendbuf[len - 1] == '\n') {
                sendbuf[len - 1] = 0;
                len--;
            }

            if (!strncmp(sendbuf, "quit", len)) {
                if (shutdown(fd, SHUT_RDWR)) {
                    error(1, errno, "shutdown");
                }
                printf("exit.\n");
                break;
            }

            char *ptr = sendbuf;
            while (len > 0) {
                int n = send(fd, ptr, len, 0);
                if (n < 0) {
                    error(1, errno, "send failed");
                } else {
                    len -= n;
                    ptr += n;
                }
            }
        }

        if (FD_ISSET(fd, &readmask)) {
            printf("get message from server:\n");
            int readn = recv(fd, recvbuf, sizeof(recvbuf) - 1, 0);
            if (readn == 0) {
                if (shutdown(fd, SHUT_RDWR)) {
                    error(1, errno, "shutdown failed");
                }
                LOG_MSG("close connection\n");
                break;
            }
            if (readn < 0) {
                continue;
            }
            recvbuf[readn] = '\0';
            printf("%s\n", recvbuf);
        }
    }

    return 0;
}