#include <dirent.h>
#include "lib/common.h"
#include "myutil.h"

static int running = 1;
void sig_handler(int signo) {
    running = 0;
}


int main(int argc, char **argv) {
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        error(1, errno, "socket error");
    }
    int on = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        error(0, errno, "sesockopt error");
    }
    struct sockaddr_in serv;
    bzero(&serv, sizeof(serv));
    serv.sin_family = AF_INET,
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port = htons(SERV_PORT);
    if (bind(fd, (struct sockaddr *)&serv, sizeof(serv)) < 0) {
        error(1, errno, "bind failed");
    }
    if (listen(fd, LISTENQ) < 0) {
        error(1, errno, "listen failed");
    }

    char recvbuf[BUF_SIZE];
    while (running) {
        struct sockaddr_in cli;
        bzero(&cli, sizeof(cli));
        socklen_t clilen;
        int connfd = accept(fd, (struct sockaddr *)&cli, &clilen);
        if (connfd < 0) {
            error(0, errno, "accept error");
            if (errno == EINTR) {
                continue;
            } else {
                break;
            }
        } else {
            char ip[INET6_ADDRSTRLEN];
            memset(ip, 0, sizeof(ip));
            yolanda_msgx("get a peer connection: [%s :%d]",
                        inet_ntop(AF_INET, &cli.sin_addr, ip, clilen), ntohs(cli.sin_port));
        }

        while (running) {
            memset(recvbuf, 0, sizeof(recvbuf));
            int n = recv(connfd, recvbuf, sizeof(recvbuf) - 1, 0);
            if (n < 0) {
                error(0, errno, "recv error");
                if (errno == EINTR) {
                    continue;
                } else {
                    if (shutdown(connfd, SHUT_RDWR) < 0) {
                        error(0, errno, "shutdown error");
                    }
                    break;
                }
            }
            if (n == 0) {
                yolanda_msgx("peerconnection closed\n");
                if (shutdown(connfd, SHUT_RDWR) < 0) {
                    error(0, errno, "shutdown error");
                }
                break;
            }
            if (recvbuf[n - 1] == '\n') {
                recvbuf[n - 1] = 0;
            }

            yolanda_msgx("get message from client: %s\n", recvbuf);
            message_process(connfd, recvbuf, strlen(recvbuf));
        }
    }

    return 0;
}
