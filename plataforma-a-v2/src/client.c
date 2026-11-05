/*
 * client.c - Cliente de terminal para o laboratorio didatico
 *
 * Disciplina : Seguranca da Informacao
 * Curso      : Engenharia de Software
 * Linguagem  : C (C99) | Windows (MinGW) e Linux/macOS
 *
 * Compilacao - Windows (MinGW):
 *   gcc -Wall -Wextra -O2 src/client.c -o client.exe -lws2_32
 *
 * Compilacao - Linux/macOS:
 *   gcc -Wall -Wextra -O2 src/client.c -o client
 *
 * Protocolo de leitura:
 *   Le um byte por vez para evitar problemas de fragmentacao do TCP.
 *   O prompt do servidor sempre termina com "> "; ao detectar esses
 *   dois bytes consecutivos, le uma linha do stdin e envia ao servidor.
 *   Carriage returns ('\r') sao descartados para compatibilidade.
 */

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define _WIN32_WINNT 0x0600
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "Ws2_32.lib")
   typedef SOCKET socket_t;
#  define CLOSESOCK closesocket
#else
#  include <arpa/inet.h>
#  include <sys/socket.h>
#  include <unistd.h>
   typedef int socket_t;
#  define INVALID_SOCKET (-1)
#  define CLOSESOCK       close
#endif

#include <stdio.h>
#include <string.h>

#define HOST "127.0.0.1"
#define PORT  5555

int main(void) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        puts("Falha ao iniciar Winsock.");
        return 1;
    }
#endif

    socket_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        puts("Falha ao criar socket.");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(PORT);

    if (inet_pton(AF_INET, HOST, &addr.sin_addr) != 1) {
        puts("Endereco IP invalido.");
        CLOSESOCK(s);
        return 1;
    }

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        puts("Conexao recusada. Certifique-se de que o servidor esta rodando.");
        CLOSESOCK(s);
        return 1;
    }

    /*
     * c0 e c1 rastreiam os dois ultimos bytes recebidos (ignorando '\r').
     * Quando c0 == '>' e c1 == ' ', o servidor exibiu o prompt e aguarda
     * entrada do usuario.
     */
    char c0 = 0, c1 = 0;

    for (;;) {
        char ch;
        int n = (int)recv(s, &ch, 1, 0);
        if (n <= 0) break;          /* servidor fechou a conexao */

        if (ch == '\r') continue;   /* descartar CR                */

        putchar(ch);
        fflush(stdout);

        c0 = c1;
        c1 = ch;

        /* Detectar prompt "> " — servidor aguarda entrada do usuario */
        if (c0 == '>' && c1 == ' ') {
            char input[256];
            if (!fgets(input, sizeof(input), stdin)) break;
            if (send(s, input, (int)strlen(input), 0) <= 0) break;
            /* Resetar rastreador para nao re-disparar no proximo byte */
            c0 = 0; c1 = 0;
        }
    }

    puts("\n[Conexao encerrada]");
    CLOSESOCK(s);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
