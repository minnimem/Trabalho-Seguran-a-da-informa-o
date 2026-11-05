# Plataforma A - Laboratorio seguro de sockets TCP

**Disciplina:** Segurança da Informação
**Professor:** Tiago Marcelino Reis  
**Curso:** Engenharia de Software  
**Integrante:** Diego de Jesus Modesto Silva  
**Matrícula:** [preencher]  
**Linguagem:** C

## Objetivo

Demonstrar uma comunicação cliente/servidor TCP com troca de mensagens e encerramento controlado. Este projeto é uma simulação defensiva e didática: o servidor escuta exclusivamente em `127.0.0.1` e implementa apenas os comandos fixos `help`, `status` e `exit`. Ele não cria shell remoto, não chama funções de execução de processos e não executa comandos recebidos.

## Estrutura

```text
src/
  client.c
  server.c
.gitignore
LICENSE
README.md
```

## Compilação e execução (Windows, MinGW)

Abra dois terminais nesta pasta. No primeiro:

```powershell
gcc -Wall -Wextra -O2 src/server.c -o server.exe -lws2_32
./server.exe
```

No segundo:

```powershell
gcc -Wall -Wextra -O2 src/client.c -o client.exe -lws2_32
./client.exe
```

Digite `help`, `status` ou `exit` no cliente. A porta TCP usada é `5555`; se estiver ocupada, encerre o processo que a utiliza antes de iniciar o laboratório.

## Demonstração de uso

```text
> help
Comandos: help, status, exit
> status
Servidor ativo; modo de simulacao; nenhum comando do SO foi executado.
> exit
Conexao encerrada com seguranca.
```

Mensagens diferentes desses três comandos recebem uma resposta de comando desconhecido e não causam execução no sistema operacional.

## Análise teórica de segurança

Um backdoor que oferece shell remoto cria um canal de comando e controle: uma conexão de rede pode dar ao operador capacidade de executar ações com os privilégios do processo comprometido. A equipe Blue Team pode reduzir o risco com regras de firewall que neguem conexões não autorizadas, monitoramento de portas e conexões de saída, inventário e análise de processos, alertas para binários ou serviços inesperados e revisão de logs. O laboratório restringe o bind ao loopback e usa uma lista fechada de mensagens, demonstrando como reduzir a superfície de ataque. A restrição ao loopback é uma medida pedagógica, não substitui controles em produção.

## Limitações

Exemplo introdutório, de conexão única, sem autenticação e sem criptografia; por isso, deve permanecer em máquina própria e ambiente controlado. O programa não deve ser exposto a redes externas. Para um sistema real, use protocolos autenticados e ferramentas de administração remota aprovadas, com autorização explícita e registro de auditoria.

## Créditos e fontes

Implementação original para fins didáticos, baseada nas interfaces padrão de sockets TCP (`socket`, `bind`, `listen`, `accept`, `connect`, `send` e `recv`). Consulte a documentação oficial da plataforma usada na disciplina para detalhes específicos de compilação.




