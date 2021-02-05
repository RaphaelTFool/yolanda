#include "myutil.h"
#include <dirent.h>
#include "lib/common.h"

void rand_str(char *buf, int buflen) {
    char charset[] = "0123456789"
                     "abcdefghijklmnopqrstuvwxyz"
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec);
    --buflen;
    while (buflen-- > 0) {
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        *buf++ = charset[index];
    }
    *buf = '\0';
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

struct buffer* message_process1(struct buffer* input) {
    struct buffer *output = buffer_new();
    char *msg = buffer_content_get(input);
    int len = buffer_readable_size(input);
    if (!strncmp(msg, "ls", len)) {
        char content[65536];
        ls(content, sizeof(content) - 1);
        buffer_append_string(output, content);
    } else if (!strncmp(msg, "pwd", len)) {
        char pwd[BUF_SIZE];
        if (getcwd(pwd, sizeof(pwd))){
            printf("%s\n", pwd);
            buffer_append_string(output, pwd);
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
            buffer_append_string(output, path);
        } else {
            yolanda_msgx("path not found\n");
        }
    } else {
        yolanda_msgx("unsupported command\n");
    }
    buffer_readn(input, len);
    return output;
}
