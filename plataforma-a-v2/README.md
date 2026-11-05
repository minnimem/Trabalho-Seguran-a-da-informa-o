# Plataforma A — Backdoor Didatico com Lista Branca de Comandos

**Disciplina:** Seguranca da Informacao
**Professor:** Tiago Marcelino Reis
**Curso:** Engenharia de Software
**Integrante:** Diego de Jesus Modesto Silva
**Matricula:** [preencher]
**Linguagem:** C (C99)

---

## 1. Objetivo

Este projeto implementa um **backdoor didatico** cliente/servidor via socket TCP, atendendo aos tres requisitos funcionais do enunciado:

1. **Conexao de rede:** socket TCP entre cliente ("atacante") e servidor ("vitima"), executavel em loopback (`127.0.0.1`) ou entre maquinas virtuais na mesma rede.
2. **Execucao de comandos:** o servidor recebe strings enviadas pela rede, valida cada uma contra uma lista branca fixa e executa o comando correspondente no sistema operacional via `popen()`, retornando a saida completa ao cliente.
3. **Mecanismo de encerramento:** os comandos `exit` e `quit` fecham a conexao de forma limpa.

### Decisao de projeto: lista branca em vez de shell arbitrario

O enunciado pede que o backdoor execute strings recebidas no SO e retorne a saida. Esta implementacao cumpre esse requisito com uma camada de seguranca: a string recebida pela rede **nunca** e passada diretamente a `system()` ou ao shell. Em vez disso, ela e comparada contra um conjunto fixo de tokens; somente o comando pre-definido correspondente e repassado ao `popen()`. Isso demonstra o mecanismo de comando-e-controle (C2) — receber string, executar no SO, retornar saida — sem criar uma ferramenta de execucao arbitraria operacional, o que seria inapropriado mesmo em contexto academico.

---

## 2. Estrutura do Repositorio

```text
/
├── src/
│   ├── server.c   (servidor / vitima)
│   └── client.c   (cliente / atacante)
├── .gitignore
├── LICENSE
└── README.md
```

---

## 3. Compilacao e Execucao

### Windows (MinGW / Git Bash)

Abra dois terminais na raiz do projeto.

**Terminal 1 — servidor:**
```powershell
gcc -Wall -Wextra -O2 src/server.c -o server.exe -lws2_32
./server.exe
```

**Terminal 2 — cliente:**
```powershell
gcc -Wall -Wextra -O2 src/client.c -o client.exe -lws2_32
./client.exe
```

> Se a porta 5555 estiver ocupada, encerre o processo com:
> `netstat -ano | findstr 5555` e depois `taskkill /PID <numero> /F`

### Linux / macOS

**Terminal 1 — servidor:**
```bash
gcc -Wall -Wextra -O2 src/server.c -o server
./server
```

**Terminal 2 — cliente:**
```bash
gcc -Wall -Wextra -O2 src/client.c -o client
./client
```

> Para verificar a porta em uso: `ss -tlnp | grep 5555` ou `lsof -i :5555`

---

## 4. Demonstracao de Uso

```
=== Backdoor Didatico | Lista Branca de Comandos ===
Digite 'help' para listar os comandos disponiveis.
> help
Comandos internos : help, status, exit, quit
Comandos do SO    : whoami, hostname, date, uptime, uname
Obs.: somente esses tokens sao autorizados; qualquer
      outra entrada e recusada sem execucao no sistema.
> whoami
diego
> hostname
DESKTOP-ABC123
> date
Qua 05 Nov 2026 00:03:14 -03
> uptime
 00:03:14 up 2:41, 1 user, load average: 0.12, 0.08, 0.06
> status
Servidor ativo | execucao restrita a lista branca | sem acesso arbitrario ao shell.
> rm -rf /
Comando nao autorizado. Nenhuma acao foi executada. Use 'help'.
> exit
Conexao encerrada com seguranca.
```

Saida do servidor durante a sessao acima:
```
[2026-11-05 00:03:10] Nova conexao aceita.
  [cmd] help
  [cmd] whoami
  [cmd] hostname
  [cmd] date
  [cmd] uptime
  [cmd] status
  [cmd] rm -rf /
  [cmd] exit
[2026-11-05 00:03:30] Sessao encerrada.
```

---

## 5. Analise Teorica de Seguranca

### Conceito explorado

Um backdoor e um canal de comando e controle (C2): um processo na maquina-alvo escuta conexoes de rede e, ao receber uma string, executa algo no sistema operacional com os privilegios do proprio processo, retornando a saida. Isso permite ao operador remoto controlar a maquina sem autorizacao do usuario. Em backdoors reais (RATs, reverse shells), qualquer string e repassada diretamente ao shell (`/bin/sh -c`, `cmd.exe /c`), dando execucao irrestrita.

### Mapeamento MITRE ATT&CK

| Tecnica | ID | Descricao |
|---|---|---|
| Command and Scripting Interpreter | T1059 | Execucao de comandos via shell remoto |
| Application Layer Protocol | T1071 | Uso de TCP comum para C2 |
| Bind Shell / Reverse Shell | T1059.004 | Servidor ouve conexao e executa comandos |

### Como a equipe Blue Team pode detectar e mitigar

**Deteccao:**
- **Monitoramento de portas abertas:** `netstat -ano` (Windows) ou `ss -tlnp` (Linux) identificam processos escutando em portas incomuns. Um processo ouvindo em uma porta que nao deveria estar aberta e um indicador de comprometimento (IoC).
- **Analise de processos:** ferramentas como Sysmon (evento 1 — Process Create, evento 3 — Network Connection) ou `ps aux` registram qual processo criou o socket. Um binario desconhecido com conexao de rede ativa e alerta imediato.
- **Captura de trafego:** Wireshark ou tcpdump revelam o conteudo em texto plano das mensagens trocadas (ausencia de criptografia e uma fraqueza proposital deste laboratorio).
- **Alertas de IDS/IPS:** regras no Snort/Suricata podem detectar padroes de texto ("Backdoor Didatico", "> ") no payload TCP.

**Mitigacao:**
- **Regras de firewall:** bloquear conexoes de entrada e saida em portas nao autorizadas; negar qualquer conexao originada de processos que nao sejam servicos conhecidos.
- **Principio do menor privilegio:** executar processos com o minimo de permissoes necessarias; mesmo que um backdoor seja implantado, seus privilegios ficam limitados.
- **Inventario e assinatura de binarios:** verificar hashes de executaveis contra uma baseline; qualquer binario novo ou modificado dispara alerta.
- **Monitoramento de logs:** centralizar logs de rede e processo em um SIEM (Splunk, Elastic) para correlacionar eventos suspeitos.
- **Restricao ao loopback:** como feito neste laboratorio, fazer bind em `127.0.0.1` impede acesso externo, reduzindo drasticamente a superficie de ataque em cenario real.

### O que este laboratorio NAO faz (diferenca para um backdoor real)

| Backdoor real | Este laboratorio |
|---|---|
| Executa qualquer comando do SO | Executa apenas tokens da lista branca |
| Passa string recebida direto ao shell | Nunca passa input do usuario ao shell |
| Pode escutar em qualquer interface | Bind exclusivo em `127.0.0.1` |
| Comunicacao criptografada (TLS/SSL) | Texto plano — intencional para analise |
| Persistencia no sistema (registro, cron) | Sem persistencia |
| Conexao reversa atraves de firewall | Bind shell local apenas |

---

## 6. Limitacoes

- Conexao unica por vez (sem threads); o servidor atende um cliente de cada vez, em sequencia.
- Sem autenticacao: qualquer processo local pode conectar na porta 5555.
- Sem criptografia: o conteudo e visivel em texto plano via captura de pacotes.
- Deve permanecer em maquina propria e ambiente controlado. Proibido usar em redes ou sistemas de terceiros sem autorizacao formal.

---

## 7. Creditos e Fontes

Implementacao original para fins didaticos, baseada nas interfaces padrao de sockets TCP (`socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`) e na funcao POSIX `popen()` para captura de saida de processos. Referencias: documentacao oficial do Winsock2 (Microsoft Docs) e das interfaces POSIX (man7.org).
