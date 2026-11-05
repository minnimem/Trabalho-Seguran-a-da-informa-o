/*
 * server.c - Servidor didatico: backdoor com lista branca de comandos
 *
 * Disciplina : Seguranca da Informacao
 * Curso      : Engenharia de Software
 * Linguagem  : C (C99) | Windows (MinGW) e Linux/macOS
 *
 * Funcionamento:
 *   - Escuta exclusivamente em 127.0.0.1:5555 (loopback).
 *   - Recebe strings do cliente pela rede via socket TCP.
 *   - Valida cada string contra uma lista branca fixa (whitelist).
 *   - Comandos internos (help, status, exit, quit): respondidos em codigo.
 *   - Comandos do SO (whoami, hostname, date, ...): executados via popen()
 *     e a saida e retornada ao cliente pela conexao de rede.
 *   - Qualquer entrada fora da whitelist e rejeitada sem nenhuma execucao.
 *   - Aceita multiplas sessoes sequenciais sem precisar reiniciar.
 *
 * Compilacao - Windows (MinGW):
 *   gcc -Wall -Wextra -O2 src/server.c -o server.exe -lws2_32
 *
 * Compilacao - Linux/macOS:
 *   gcc -Wall -Wextra -O2 src/server.c -o server
 */

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define _WIN32_WINNT 0x0600
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "Ws2_32.lib")
   typedef SOCKET socket_t;
#  define CLOSESOCK  closesocket
#  define popen      _popen
#  define pclose     _pclose
#else
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <unistd.h>
   typedef int socket_t;
#  define INVALID_SOCKET (-1)
#  define SOCKET_ERROR   (-1)
#  define CLOSESOCK       close
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>

#define PORT        5555
#define BUF_INPUT    512   /* tamanho maximo do comando recebido */
#define BUF_OUTPUT  8192   /* tamanho maximo da saida capturada  */

/* =========================================================================
 * Lista branca: token que o usuario digita -> comando executado no SO.
 * A string recebida pela rede NUNCA e passada diretamente ao shell;
 * apenas o campo 'syscmd' fixo e repassado ao popen().
 * ========================================================================= */
typedef struct {
    const char *token;   /* o que o cliente envia */
    const char *syscmd;  /* o que e executado no SO */
} WhiteEntry;

static const WhiteEntry WHITELIST[] = {
#ifdef _WIN32
    { "whoami",   "whoami"   },
    { "hostname", "hostname" },
    { "date",     "date /t"  },
    { "time",     "time /t"  },
    { "ver",      "ver"      },
#else
    { "whoami",   "whoami"   },
    { "hostname", "hostname" },
    { "date",     "date"     },
    { "uptime",   "uptime"   },
    { "uname",    "uname -r" },
#endif
    { NULL, NULL }
};

/* =========================================================================
 * Envia todos os bytes de 'data' garantindo envio parcial.
 * ========================================================================= */
static int send_all(socket_t s, const char *data, size_t len) {
    while (len > 0) {
        int n = (int)send(s, data, (int)len, 0);
        if (n <= 0) return 0;
        data += n;
        len  -= (size_t)n;
    }
    return 1;
}

static int send_str(socket_t s, const char *str) {
    return send_all(s, str, strlen(str));
}

/* =========================================================================
 * Executa 'syscmd' via popen(), captura a saida e envia ao cliente.
 * ========================================================================= */
static void exec_and_send(socket_t s, const char *syscmd) {
    char out[BUF_OUTPUT];
    size_t total = 0;

    FILE *fp = popen(syscmd, "r");
    if (!fp) {
        send_str(s, "[Erro: nao foi possivel executar o comando.]\n");
        return;
    }

    while (total < sizeof(out) - 1) {
        size_t n = fread(out + total, 1, sizeof(out) - 1 - total, fp);
        if (n == 0) break;
        total += n;
    }
    pclose(fp);

    out[total] = '\0';
    if (total == 0)
        send_str(s, "[Sem saida.]\n");
    else
        send_all(s, out, total);
}

/* =========================================================================
 * Atende uma sessao de cliente ate o encerramento.
 * ========================================================================= */
static void handle_session(socket_t peer) {
    const char *welcome =
        "=== Backdoor Didatico | Lista Branca de Comandos ===\n"
        "Digite 'help' para listar os comandos disponiveis.\n> ";
    send_str(peer, welcome);

    char buf[BUF_INPUT];
    size_t used = 0;

    while (1) {
        char ch;
        int n = (int)recv(peer, &ch, 1, 0);
        if (n <= 0) break;          /* cliente desconectou ou erro de rede */

        if (ch == '\r') continue;   /* descartar CR (terminais Windows)    */

        if (ch != '\n') {
            /* Acumular caracteres; truncar silenciosamente se muito longa  */
            if (used + 1 < sizeof(buf)) buf[used++] = ch;
            continue;
        }

        /* Fim de linha: processar comando acumulado */
        buf[used] = '\0';
        used = 0;

        /* Ignorar linha vazia — apenas reexibir o prompt */
        if (buf[0] == '\0') {
            send_str(peer, "> ");
            continue;
        }

        printf("  [cmd] %s\n", buf);

        /* -----------------------------------------------------------------
         * Comandos internos
         * ----------------------------------------------------------------- */
        if (strcmp(buf, "help") == 0) {
            send_str(peer,
                "Comandos internos : help, status, exit, quit\n"
#ifdef _WIN32
                "Comandos do SO    : whoami, hostname, date, time, ver\n"
#else
                "Comandos do SO    : whoami, hostname, date, uptime, uname\n"
#endif
                "Obs.: somente esses tokens sao autorizados; qualquer\n"
                "      outra entrada e recusada sem execucao no sistema.\n> "
            );
            continue;
        }

        if (strcmp(buf, "status") == 0) {
            send_str(peer,
                "Servidor ativo | execucao restrita a lista branca | "
                "sem acesso arbitrario ao shell.\n> ");
            continue;
        }

        if (strcmp(buf, "exit") == 0 || strcmp(buf, "quit") == 0) {
            send_str(peer, "Conexao encerrada com seguranca.\n");
            break;
        }

        /* -----------------------------------------------------------------
         * Comandos do SO — busca na whitelist
         * ----------------------------------------------------------------- */
        int found = 0;
        for (int i = 0; WHITELIST[i].token != NULL; i++) {
            if (strcmp(buf, WHITELIST[i].token) == 0) {
                exec_and_send(peer, WHITELIST[i].syscmd);
                send_str(peer, "> ");
                found = 1;
                break;
            }
        }

        if (!found) {
            send_str(peer,
                "Comando nao autorizado. Nenhuma acao foi executada. "
                "Use 'help'.\n> ");
        }
    }
}

/* =========================================================================
 * main: inicializa o socket e entra em loop de accept().
 * ========================================================================= */
int main(void) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        puts("Falha ao iniciar Winsock.");
        return 1;
    }
#endif

    socket_t listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == INVALID_SOCKET) {
        puts("Falha ao criar socket.");
        return 1;
    }

    /*
     * Reusar a porta facilita o reinicio rapido do servidor.
     * Windows: SO_EXCLUSIVEADDRUSE — impede sequestro de porta.
     * Linux  : SO_REUSEADDR        — permite rebind apos TIME_WAIT.
     */
#ifdef _WIN32
    int excl = 1;
    setsockopt(listener, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
               (char *)&excl, sizeof(excl));
#else
    int reuse = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(PORT);

    /* Verificar retorno de inet_pton — falha silenciosa em MinGW antigo */
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1) {
        puts("Endereco IP invalido.");
        CLOSESOCK(listener);
        return 1;
    }

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR
        || listen(listener, 1) == SOCKET_ERROR) {
        puts("Falha em bind/listen. A porta 5555 esta ocupada?");
        CLOSESOCK(listener);
        return 1;
    }

    printf("Servidor didatico escutando em 127.0.0.1:%d\n", PORT);
    puts("Aguardando conexoes (Ctrl+C para encerrar)...\n");

    /* Loop de sessoes: aceita um cliente por vez, em sequencia */
    for (;;) {
        socket_t peer = accept(listener, NULL, NULL);
        if (peer == INVALID_SOCKET) continue;

        char ts[32];
        time_t now = time(NULL);
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
        printf("[%s] Nova conexao aceita.\n", ts);

        handle_session(peer);

        CLOSESOCK(peer);
        now = time(NULL);
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
        printf("[%s] Sessao encerrada.\n\n", ts);
    }

    /* Ponto de saida nunca atingido em operacao normal (Ctrl+C) */
    CLOSESOCK(listener);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
