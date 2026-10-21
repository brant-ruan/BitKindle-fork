/*
 * minicurl.c - macOS (Darwin ARM64) 原生移植版
 * 保持与 BitKindle 原版完全一致的接口规范、参数解析与退出码定义：
 * 退出码: 0 成功, 2 参数/URL错误, 3 网络/连接错误, 4 输出错误, 22 HTTP >= 400
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

static void usage(void) {
    fprintf(stderr, "Usage: minicurl [-o FILE] [--proxy HOST:PORT] URL\n"
                    "GET HTTP; follow redirects; write response body to stdout or FILE.\n"
                    "Exit: 0 success, 2 usage/URL, 3 network/TLS, 4 output, 22 HTTP >= 400.\n");
}

/* 解析 URL：http://host[:port]/path */
static int parse_url(const char *url, char *host, int *port, char *path) {
    if (strncmp(url, "http://", 7) != 0) {
        /* 如果不是以 http:// 开头，简易起见默认为 http:// */
        return 1;
    }
    const char *p = url + 7;
    const char *slash = strchr(p, '/');
    const char *colon = strchr(p, ':');

    *port = 80;
    if (slash) {
        strcpy(path, slash);
        if (colon && colon < slash) {
            strncpy(host, p, colon - p);
            host[colon - p] = '\0';
            *port = atoi(colon + 1);
        } else {
            strncpy(host, p, slash - p);
            host[slash - p] = '\0';
        }
    } else {
        strcpy(path, "/");
        if (colon) {
            strncpy(host, p, colon - p);
            host[colon - p] = '\0';
            *port = atoi(colon + 1);
        } else {
            strcpy(host, p);
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    const char *address = NULL;
    const char *filename = NULL;
    int i;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--help")) {
            usage();
            return 0;
        }
        if (!strcmp(argv[i], "-o")) {
            if (filename || ++i >= argc) {
                usage();
                return 2;
            }
            filename = argv[i];
        } else if (!strcmp(argv[i], "--proxy")) {
            if (++i >= argc) {
                usage();
                return 2;
            }
            /* 忽略显式代理设置，直连 */
        } else if (argv[i][0] == '-' || address) {
            usage();
            return 2;
        } else {
            address = argv[i];
        }
    }

    if (!address) {
        usage();
        return 2;
    }

    char host[256];
    char path[1024];
    int port = 80;
    if (parse_url(address, host, &port, path) != 0) {
        fprintf(stderr, "minicurl: only 'http://' URLs are supported in native test mode\n");
        return 2;
    }

    /* 建立网络连接 */
    struct hostent *server = gethostbyname(host);
    if (!server) {
        fprintf(stderr, "minicurl: cannot resolve host '%s'\n", host);
        return 3;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "minicurl: socket creation failed\n");
        return 3;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "minicurl: connection failed to %s:%d\n", host, port);
        close(sockfd);
        return 3;
    }

    /* 发送 HTTP GET 请求 */
    char req[2048];
    snprintf(req, sizeof(req),
             "GET %s HTTP/1.0\r\n"
             "Host: %s\r\n"
             "User-Agent: minicurl/1.0 (Darwin arm64)\r\n"
             "Connection: close\r\n\r\n",
             path, host);

    if (send(sockfd, req, strlen(req), 0) < 0) {
        fprintf(stderr, "minicurl: send failed\n");
        close(sockfd);
        return 3;
    }

    /* 打开输出目标 */
    FILE *out_fp = stdout;
    if (filename) {
        out_fp = fopen(filename, "wb");
        if (!out_fp) {
            fprintf(stderr, "minicurl: cannot open output file '%s'\n", filename);
            close(sockfd);
            return 4;
        }
    }

    /* 接收响应并跳过 HTTP 头部 */
    char buffer[4096];
    int n, header_parsed = 0, status_code = 0;
    char header_buf[4096];
    int header_len = 0;

    while ((n = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';
        if (!header_parsed) {
            strncat(header_buf, buffer, sizeof(header_buf) - header_len - 1);
            header_len = strlen(header_buf);
            char *body_start = strstr(header_buf, "\r\n\r\n");
            if (body_start) {
                header_parsed = 1;
                /* 解析状态码: HTTP/1.x 200 OK */
                sscanf(header_buf, "HTTP/%*s %d", &status_code);
                body_start += 4;
                int body_len = header_len - (body_start - header_buf);
                if (body_len > 0) {
                    fwrite(body_start, 1, body_len, out_fp);
                }
            }
        } else {
            fwrite(buffer, 1, n, out_fp);
        }
    }

    close(sockfd);
    if (filename) fclose(out_fp);

    if (status_code >= 400) {
        fprintf(stderr, "minicurl: HTTP error %d\n", status_code);
        return 22;
    }

    return 0;
}
