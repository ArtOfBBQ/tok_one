/*
An example web server for trying T1's web builds locally

concat builds it into build/web/ when this file is newer than the server
there.

run:
    ./build/web/serve build/web/examples
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET Socket;
#define SOCKET_BAD INVALID_SOCKET
#define close_socket closesocket
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int Socket;
#define SOCKET_BAD (-1)
#define close_socket close
#endif

#define REQUEST_CAP 8192
#define PATH_CAP 1024

static const char * content_type(const char * path) {
    static const char * types[][2] = {
        {".html", "text/html; charset=utf-8"},
        {".js", "text/javascript; charset=utf-8"},
        {".wasm", "application/wasm"},
        {".txt", "text/plain; charset=utf-8"},
        {".json", "application/json"},
        {".css", "text/css"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".svg", "image/svg+xml"},
    };
    const char * dot = strrchr(path, '.');
    if (dot != NULL) {
        for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
            if (strcmp(dot, types[i][0]) == 0) { return types[i][1]; }
        }
    }
    return "application/octet-stream";
}

static void send_all(Socket client, const char * data, size_t size) {
    while (size > 0) {
        int sent = (int)send(client, data, (int)size, 0);
        if (sent <= 0) { return; }
        data += sent;
        size -= (size_t)sent;
    }
}

static void respond_status(Socket client, const char * status) {
    char response[256];
    int size = snprintf(
        response,
        sizeof(response),
        "HTTP/1.1 %s\r\nContent-Type: text/plain\r\n"
        "Content-Length: %zu\r\nConnection: close\r\n\r\n%s",
        status,
        strlen(status),
        status);
    send_all(client, response, (size_t)size);
}

// a URL's %XX escapes as the bytes they are, then a newline
static void print_url_decoded(const char * text) {
    for (const char * at = text; *at != '\0'; at++) {
        unsigned int byte = 0;
        if (*at == '%' && sscanf(at + 1, "%2x", &byte) == 1) {
            putchar((int)byte);
            at += 2;
        } else {
            putchar(*at == '+' ? ' ' : *at);
        }
    }
    putchar('\n');
}

static void serve_one(Socket client, const char * root) {
    char request[REQUEST_CAP];
    int received = (int)recv(client, request, REQUEST_CAP - 1, 0);
    if (received <= 0) { return; }
    request[received] = '\0';

    // "GET /path?query HTTP/1.1"
    char method[16];
    char url[PATH_CAP];
    if (sscanf(request, "%15s %1023s", method, url) != 2) {
        respond_status(client, "400 Bad Request");
        return;
    }
    if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0) {
        respond_status(client, "405 Method Not Allowed");
        return;
    }
    char * query = strchr(url, '?');
    if (query != NULL) { *query = '\0'; }
    if (strcmp(url, "/t1_log") == 0) {
        print_url_decoded(query != NULL ? query + 1 : "");
        respond_status(client, "200 OK");
        return;
    }
    // nothing outside the folder
    if (url[0] != '/' || strstr(url, "..") != NULL || strchr(url, '\\')) {
        respond_status(client, "403 Forbidden");
        return;
    }

    char path[PATH_CAP * 2];
    snprintf(
        path,
        sizeof(path),
        "%s%s%s",
        root,
        url,
        url[strlen(url) - 1] == '/' ? "index.html" : "");

    FILE * file = fopen(path, "rb");
    if (file == NULL) {
        printf("404 %s\n", url);
        respond_status(client, "404 Not Found");
        return;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char header[1024];
    int header_size = snprintf(
        header,
        sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Cache-Control: no-store\r\n"
        "Cross-Origin-Opener-Policy: same-origin\r\n"
        "Cross-Origin-Embedder-Policy: require-corp\r\n"
        "Connection: close\r\n\r\n",
        content_type(path),
        size);
    send_all(client, header, (size_t)header_size);

    if (strcmp(method, "GET") == 0) {
        char chunk[65536];
        size_t read;
        while ((read = fread(chunk, 1, sizeof(chunk), file)) > 0) {
            send_all(client, chunk, read);
        }
    }
    fclose(file);
}

int main(int argc, char ** argv) {
    if (argc < 2 || argc > 3) {
        printf("usage: ./serve folder [port]\n");
        return 1;
    }
    const char * root = argv[1];
    int port = argc == 3 ? atoi(argv[2]) : 8000;

    #if defined(_WIN32)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Couldn't start Winsock\n");
        return 1;
    }
    #else
    // a browser that hangs up mid-file mustn't end the server
    signal(SIGPIPE, SIG_IGN);
    #endif

    Socket server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == SOCKET_BAD) {
        printf("Couldn't make a socket\n");
        return 1;
    }
    int yes = 1;
    setsockopt(
        server, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons((uint16_t)port);
    // localhost only: nobody else on the network can connect
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (
        bind(server, (struct sockaddr *)&address, sizeof(address)) != 0 ||
        listen(server, 16) != 0)
    {
        printf("Couldn't listen on port %d (is a server running there?)\n",
            port);
        return 1;
    }

    printf("Serving %s at http://localhost:%d (Ctrl+C stops)\n", root, port);
    fflush(stdout);
    while (1) {
        Socket client = accept(server, NULL, NULL);
        if (client == SOCKET_BAD) { continue; }
        serve_one(client, root);
        close_socket(client);
        fflush(stdout);
    }
}
