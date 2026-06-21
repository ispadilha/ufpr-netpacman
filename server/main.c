#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../shared/socket.h"
#include "../shared/protocol.h"
#include "game.h"
#include "movements.h"
#include "logger.h"

#define MAPA_DEFAULT "assets/maps/default.csv"
#define MAPA_CUSTOM "assets/maps/custom.csv"
#define TIMEOUT_MS 1000
#define MAX_TENTATIVAS 30

static int eh_movimento(unsigned char tipo)
{
    return tipo == MSG_DIREITA || tipo == MSG_ESQUERDA || tipo == MSG_CIMA || tipo == MSG_BAIXO;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Uso: %s <interface>\n", argv[0]);
        return 1;
    }

    int soquete = cria_raw_socket(argv[1]);
    log_info("Servidor iniciado, aguardando MSG_INIT...");

    // Espera o cliente enviar o INIT
    PacmanPacket pkt;
    while (1) {
        if (recebe_mensagem(soquete, TIMEOUT_MS, &pkt) > 0) {
            log_recebido(&pkt);
            if (pkt.tipo == MSG_INIT) break;
            log_info("Pacote ignorado (não é INIT).");
        }
    }

    // Usa o mapa customizado se existir
    int usa_custom = (access(MAPA_CUSTOM, R_OK) == 0);
    const char *mapa = usa_custom ? MAPA_CUSTOM : MAPA_DEFAULT;

    // Inicializa um estado inicial de jogo
    GameState jogo;
    int ini = jogo_inicializa(mapa, &jogo);
    if (ini != 0) {
        PacmanPacket erro = {.tamanho = 0, .sequencia = pkt.sequencia, .tipo = MSG_ERRO};
        envia_mensagem(soquete, &erro);
        log_enviado(&erro);
        log_erro(ini == -1 ? "Erro ao carregar o mapa. Encerrando."
                           : "Não foi gerado um tabuleiro válido. Encerrando.");
        close(soquete);
        return 1;
    }

    // Confirma o INIT (envia um ACK)
    PacmanPacket ack_init = {.tamanho = 0, .sequencia = pkt.sequencia, .tipo = MSG_ACK};
    envia_mensagem(soquete, &ack_init);
    log_enviado(&ack_init);

    // Envia a primeira visualização
    unsigned char janela[64];
    PacmanPacket vis = {.sequencia = 0, .tipo = MSG_VISUALIZACAO, .dados = janela};
    jogo_monta_janela(&jogo, janela, &vis.tamanho);
    envia_com_ack(soquete, &vis, TIMEOUT_MS, MAX_TENTATIVAS);
    log_enviado(&vis);

    jogo_renderiza_servidor(&jogo);

    // Inicializa variáveis para sequenciamento de pacotes
    int ultima_seq_cmd = -1; // .sequencia do último comando recebido
    unsigned char seq_vis = 1; // .sequencia pra próxima visualização enviada

    // Loop do jogo
    while (1) {
        PacmanPacket cmd;

        // Aguarda um comando do jogador
        if (recebe_mensagem(soquete, TIMEOUT_MS, &cmd) < 0)
            continue;

        // Client pediu para encerrar: confirma e finaliza
        if (cmd.tipo == MSG_FIM_TRANS)
        {
            PacmanPacket ack = {.tamanho = 0, .sequencia = cmd.sequencia, .tipo = MSG_ACK};
            envia_mensagem(soquete, &ack);
            log_recebido(&cmd);
            log_info("Client encerrou o jogo. Finalizando.");
            break;
        }

        if (!eh_movimento(cmd.tipo))
            continue;
        log_recebido(&cmd);

        // Antes de qualquer coisa: confirma o recebimento
        PacmanPacket ack = {.tamanho = 0, .sequencia = cmd.sequencia, .tipo = MSG_ACK};
        envia_mensagem(soquete, &ack);

        // Comando repetido (o cliente não recebeu a visualização anterior):
        // reenvia a última janela, sem reprocessar o movimento
        if ((int)cmd.sequencia == ultima_seq_cmd)
        {
            envia_com_ack(soquete, &vis, TIMEOUT_MS, MAX_TENTATIVAS);
            continue;
        }
        ultima_seq_cmd = cmd.sequencia;

        // Processa o movimento do pacman
        jogo.movimentos++;
        move_pacman(&jogo, tipo_para_direcao(cmd.tipo));

        // Se coletou todas as pastilhas: vitória (nem precisa mover os fantasmas)
        if (jogo_venceu(&jogo)) {
            jogo_renderiza_servidor(&jogo);
            const char *texto = "Voce venceu!";
            PacmanPacket fim = {
                .tamanho = (unsigned char)strlen(texto),
                .sequencia = seq_vis,
                .tipo = MSG_FIM_TRANS,
                .dados = (unsigned char *)texto,
            };
            envia_com_ack(soquete, &fim, TIMEOUT_MS, MAX_TENTATIVAS);
            log_enviado(&fim);
            log_info("Fim de jogo: vitoria (todas as pastilhas coletadas).");
            break;
        }

        move_fantasmas(&jogo);
        jogo_renderiza_servidor(&jogo);

        // Se algum fantasma chegou a uma das 8 casas adjacentes ao pacman: derrota
        if (jogo_perdeu(&jogo)) {
            const char *texto = "Fim de jogo!";
            PacmanPacket fim = {
                .tamanho = (unsigned char)strlen(texto),
                .sequencia = seq_vis,
                .tipo = MSG_FIM_TRANS,
                .dados = (unsigned char *)texto,
            };
            envia_com_ack(soquete, &fim, TIMEOUT_MS, MAX_TENTATIVAS);
            log_enviado(&fim);
            log_info("Fim de jogo: derrota (um fantasma alcancou o pacman).");
            break;
        }

        // Envia a nova visualização
        vis.sequencia = seq_vis;
        seq_vis = (seq_vis + 1) % 64; // sequencia tem 6 bits: volta a 0 após 63
        jogo_monta_janela(&jogo, janela, &vis.tamanho);
        envia_com_ack(soquete, &vis, TIMEOUT_MS, MAX_TENTATIVAS);
        log_enviado(&vis);
    }

    close(soquete);
    return 0;
}
