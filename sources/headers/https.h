#include <3ds.h>
#include <string>
#include <netdb.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <citro2d.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "urlspecial.h"

//comment!!

static u32 *socBuffer = NULL;
static int sock_fd = -1;

//no idea
size_t net_read(void *ptr, size_t size, size_t nmemb, void *datasource) {
    int fd = *(int*)datasource;
    ssize_t res = recv(fd, ptr, size * nmemb, 0);
    if (res <= 0) return 0;
    return res / size;
}

//pretty sure this closes a connection, not sure
int net_close(void *datasource) {
    int fd = *(int*)datasource;
    if (fd >= 0) close(fd);
    return 0;
}

//searches for songs via python backend
int search_http(char* searchids, size_t id_sizes, const char* ip, int port, const char* search_id) {
    struct sockaddr_in serv_addr;
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) return -1;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);

    if (connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock_fd);
        sock_fd = -1;
        return -1;
    }
    char searchid[64];
    url_encode_string(search_id, searchid, 64*sizeof(char));
    printf("%s\n", searchid);

    char request[512];
    snprintf(request, sizeof(request),
        "GET /search/?q=%s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "User-Agent: 3DS-Spotify/1.0\r\n"
        "Connection: close\r\n\r\n",
        searchid, ip, port);

    send(sock_fd, request, strlen(request), 0);

    char buffer[1];
    int header_state = 0;
    while (recv(sock_fd, buffer, 1, 0) > 0) {
        if ((header_state == 0 || header_state == 2) && buffer[0] == '\r') header_state++;
        else if ((header_state == 1 || header_state == 3) && buffer[0] == '\n') header_state++;
        else header_state = 0;

        if (header_state == 4) break;
    }
    char* offset = searchids;
    int recvd = 1;
    while (recvd > 0) {
        recvd = recv(sock_fd, offset, id_sizes-(offset-searchids), 0);
        offset += recvd;
    };
    close(sock_fd);
    return (offset-searchids);
}

//i think it gets a track
int connect_http(const char* ip, int port, const char* track_id) {
    struct sockaddr_in serv_addr;
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) return -1;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);

    if (connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock_fd);
        sock_fd = -1;
        return -1;
    }

    char request[512];
    snprintf(request, sizeof(request),
        "GET /music/spotify:track:%s/ HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "User-Agent: 3DS-Spotify/1.0\r\n"
        "Connection: close\r\n\r\n",
        track_id, ip, port);

    send(sock_fd, request, strlen(request), 0);

    char buffer[1];
    int header_state = 0;
    while (recv(sock_fd, buffer, 1, 0) > 0) {
        if ((header_state == 0 || header_state == 2) && buffer[0] == '\r') header_state++;
        else if ((header_state == 1 || header_state == 3) && buffer[0] == '\n') header_state++;
        else header_state = 0;

        if (header_state == 4) break;
    }

    return sock_fd;
}