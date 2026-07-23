#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFFER_SIZE 4096

void print_usage() {
    printf("Usage: tsukine_iwr <URL> [-o output_file]\n");
    printf("  URL          : http://example.com/file.txt\n");
    printf("  -o output    : Output file name (default: based on URL)\n");
}

int parse_url(const char *url, char *host, char *path) {
    // 跳过 http://
    const char *start = url;
    if (strncmp(url, "http://", 7) == 0) {
        start = url + 7;
    } else {
        printf("Error: Only HTTP is supported.\n");
        return -1;
    }

    char *slash = strchr(start, '/');
    if (slash) {
        strncpy(host, start, slash - start);
        host[slash - start] = '\0';
        strcpy(path, slash);
    } else {
        strcpy(host, start);
        strcpy(path, "/");
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    char *url = argv[1];
    char *output_file = NULL;

    // 解析参数
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        }
    }

    char host[256], path[256];
    if (parse_url(url, host, path) < 0) return 1;

    // 如果没有指定输出文件名，从 URL 里取
    char default_output[256];
    if (!output_file) {
        char *last_slash = strrchr(path, '/');
        if (last_slash && *(last_slash + 1) != '\0') {
            strcpy(default_output, last_slash + 1);
        } else {
            strcpy(default_output, "index.html");
        }
        output_file = default_output;
    }

    // DNS 解析
    struct hostent *server = gethostbyname(host);
    if (!server) {
        printf("Error: Cannot resolve host: %s\n", host);
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr, server->h_addr, server->h_length);
    addr.sin_port = htons(80);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    // 发送 HTTP GET 请求
    char request[512];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.0\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             path, host);

    if (send(sock, request, strlen(request), 0) < 0) {
        perror("send");
        close(sock);
        return 1;
    }

    // 接收并写入文件
    FILE *out = fopen(output_file, "w");
    if (!out) {
        perror("fopen");
        close(sock);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    int bytes;
    int body_started = 0;
    int header_end_found = 0;
    int total_bytes = 0;

    while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes] = '\0';

        if (!header_end_found) {
            char *header_end = strstr(buffer, "\r\n\r\n");
            if (header_end) {
                header_end_found = 1;
                char *body = header_end + 4;
                int body_len = bytes - (body - buffer);
                if (body_len > 0) {
                    fwrite(body, 1, body_len, out);
                    total_bytes += body_len;
                }
            }
        } else {
            fwrite(buffer, 1, bytes, out);
            total_bytes += bytes;
        }
    }

    fclose(out);
    close(sock);

    printf("Downloaded %d bytes to %s\n", total_bytes, output_file);
    return 0;
}
