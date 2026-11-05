/* Cliente de terminal para o laboratorio didatico local. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
typedef SOCKET socket_t;
#define CLOSESOCK closesocket
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int socket_t;
#define INVALID_SOCKET (-1)
#define CLOSESOCK close
#endif
#include <stdio.h>
#include <string.h>

int main(void) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { puts("Falha ao iniciar Winsock."); return 1; }
#endif
    socket_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) { puts("Falha ao criar socket."); return 1; }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr)); addr.sin_family = AF_INET; addr.sin_port = htons(5555);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) { puts("Nao conectou. Inicie o servidor primeiro."); CLOSESOCK(s); return 1; }
    char buf[512]; int n;
    while ((n = (int)recv(s, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0'; fputs(buf, stdout);
        if (strstr(buf, "Conexao encerrada com seguranca.")) break;
        if (strstr(buf, "> ")) {
            char line[256];
            if (!fgets(line, sizeof(line), stdin)) break;
            send(s, line, (int)strlen(line), 0);
        }
    }
    CLOSESOCK(s);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
