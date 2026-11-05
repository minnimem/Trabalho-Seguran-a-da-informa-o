/* Laboratorio didatico de sockets TCP. Aceita apenas comandos fixos. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
typedef SOCKET socket_t;
#define CLOSESOCK closesocket
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int socket_t;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define CLOSESOCK close
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 5555
#define BUFFER_SIZE 256

static int send_all(socket_t s, const char *data, size_t len) {
    while (len) {
        int n = (int)send(s, data, (int)len, 0);
        if (n <= 0) return 0;
        data += n; len -= (size_t)n;
    }
    return 1;
}

int main(void) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { puts("Falha ao iniciar Winsock."); return 1; }
#endif
    socket_t listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == INVALID_SOCKET) { puts("Falha ao criar socket."); return 1; }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr); /* Somente loopback. */
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR || listen(listener, 1) == SOCKET_ERROR) {
        puts("Falha em bind/listen. Verifique se a porta 5555 esta livre."); CLOSESOCK(listener); return 1;
    }
    printf("Servidor didatico ouvindo apenas em 127.0.0.1:%d\n", PORT);
    puts("Aguardando cliente; encerre com Ctrl+C.");
    socket_t peer = accept(listener, NULL, NULL);
    if (peer == INVALID_SOCKET) { puts("Falha ao aceitar cliente."); CLOSESOCK(listener); return 1; }
    const char *welcome = "Laboratorio seguro. Digite help, status ou exit.\n> ";
    send_all(peer, welcome, strlen(welcome));
    char buf[BUFFER_SIZE];
    size_t used = 0;
    int running = 1;
    while (running) {
        char ch;
        int n = (int)recv(peer, &ch, 1, 0);
        if (n <= 0) break;
        if (ch != '\n' && ch != '\r') { if (used + 1 < sizeof(buf)) buf[used++] = ch; continue; }
        buf[used] = '\0'; used = 0;
        const char *reply;
        if (strcmp(buf, "help") == 0) reply = "Comandos: help, status, exit\n> ";
        else if (strcmp(buf, "status") == 0) reply = "Servidor ativo; modo de simulacao; nenhum comando do SO foi executado.\n> ";
        else if (strcmp(buf, "exit") == 0) { reply = "Conexao encerrada com seguranca.\n"; running = 0; }
        else reply = "Comando desconhecido. Use help. Nenhuma acao foi executada.\n> ";
        if (!send_all(peer, reply, strlen(reply))) break;
    }
    CLOSESOCK(peer); CLOSESOCK(listener);
#ifdef _WIN32
    WSACleanup();
#endif
    puts("Servidor finalizado.");
    return 0;
}
