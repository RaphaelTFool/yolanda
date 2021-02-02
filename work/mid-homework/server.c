#include <dirent.h>
#include "lib/common.h"

#define BUF_SIZE 4096

static int running = 1;
void sig_handler(int signo) {
    running = 0;
}

void trim_space(char *orig) {
    char *ptr = orig;
    while (*orig) {
        if (*orig != ' ') {
            *ptr = *orig;
            ptr++;
        }
        orig++;
    }
    *ptr = '\0';
}

void ls(char *buf, int len) {
    DIR *pdir;
    struct dirent *pdirent;
    char *ptr = buf;

    if (!(pdir = opendir("."))) {
        error(0, errno, "ls error");
    } else {
        while ((pdirent = readdir(pdir))) {
            if (!strcmp(pdirent->d_name, ".")
            || !strcmp(pdirent->d_name, "..")) {
                continue;
            }
            int dlen = strlen(pdirent->d_name);
            if (len - 1 > dlen) {
                strncpy(buf, pdirent->d_name, dlen);
                buf[dlen] = '\n';
                buf += dlen + 1;
                len -= dlen + 1;
            }
        }
        *buf = 0;
    }
}

int tcp_send(int fd, char *buf, int len) {
    int ret = 0;
    int nleft = len;
    while (nleft > 0) {
        int n = send(fd, buf, len, 0);
        if (n < 0) {
            if (errno == EINTR || errno == EWOULDBLOCK) {
                continue;
            } else {
                error(0, errno, "send error");
                break;
            }
        } else {
            buf += n;
            nleft -= n;
        }
    }
    return len - nleft;
}

void message_process(int connfd, char *msg, int len) {
    if (!strncmp(msg, "ls", len)) {
        char content[65536];
        ls(content, sizeof(content) - 1);
        int sendn = strlen(content);
        if (sendn != tcp_send(connfd, content, sendn)) {
            yolanda_msgx("send msg error\n");
        }
    } else if (!strncmp(msg, "pwd", len)) {
        char pwd[BUF_SIZE];
        if (getcwd(pwd, sizeof(pwd))){
            printf("%s\n", pwd);
            int sendn = strlen(pwd);
            if (sendn != tcp_send(connfd, pwd, sendn)) {
                yolanda_msgx("send msg error\n");
            }
        } else {
            error(0, errno, "getcwd failed");
        }
    } else if (!strncmp(msg, "cd ", 3)) {
        char *path = msg+ 3;
        printf("path: %s\n", path);
        trim_space(path);
        printf("path: %s\n", path);
        if (strlen(path) == 1) {
            switch (*path) {
                case '-':
                path = getenv("OLDPWD");
                break;
                case '~':
                path = getenv("HOME");
                break;
            }
        }
        if (path) {
            printf("path: %s\n", path);
            if (chdir(path) < 0) {
                error(0, errno, "chdir failed");
            }
        } else {
            yolanda_msgx("path not found\n");
        }
    } else {
        yolanda_msgx("unsupported command\n");
    }
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
