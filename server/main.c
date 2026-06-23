#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../shared/socket.h"
#include "../shared/protocol.h"
#include "../shared/transfer.h"
#include "../shared/screen.h"
#include "game.h"
#include "movements.h"
#include "logger.h"

#define MAPA_DEFAULT "assets/maps/default.csv"
#define MAPA_CUSTOM "assets/maps/custom.csv"
#define IMAGEM_GAME_OVER "assets/game_over.jpg"
#define TIMEOUT_MS 1000
#define MAX_TENTATIVAS 30

static int eh_movimento(unsigned char tipo)
{
    return tipo == MSG_DIREITA || tipo == MSG_ESQUERDA || tipo == MSG_CIMA || tipo == MSG_BAIXO;
}

static void monta_caminho_premio(int d, char *caminho, char *nome, unsigned char *tipo)
{
    const char *ext;
    if (d <= 2)
    {
        *tipo = MSG_TXT;
        ext = "txt";
    }
    else if (d <= 4)
    {
        *tipo = MSG_JPG;
        ext = "jpg";
    }
    else
    {
        *tipo = MSG_MP4;
        ext = "mp4";
    }

    sprintf(nome, "%d.%s", d, ext);
    sprintf(caminho, "assets/%s", nome);
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
    unsigned char janela[MAX_JANELA];
    int total_vis;
    unsigned char seq_envio = 0; // .sequencia do próximo pacote enviado
    jogo_monta_janela(&jogo, janela, &total_vis);
    transfer_envia_visualizacao(soquete, janela, total_vis, &seq_envio, TIMEOUT_MS, MAX_TENTATIVAS);
    log_info("Visualizacao inicial enviada.");

    int ultima_seq_cmd = -1; // .sequencia do último comando recebido

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
            transfer_envia_visualizacao(soquete, janela, total_vis, &seq_envio, TIMEOUT_MS, MAX_TENTATIVAS);
            continue;
        }
        ultima_seq_cmd = cmd.sequencia;

        // Processa o movimento do pacman
        jogo.movimentos++;
        int coletada = move_pacman(&jogo, tipo_para_direcao(cmd.tipo));

        // Se coletou uma pastilha: avisa e envia o arquivo prêmio
        if (coletada)
        {
            char caminho[32], nome[16];
            unsigned char tipo_premio;
            monta_caminho_premio(coletada, caminho, nome, &tipo_premio);

            PacmanPacket aviso_premio = {
                .tamanho = (unsigned char)strlen(nome),
                .sequencia = seq_envio,
                .tipo = tipo_premio,
                .dados = (unsigned char *)nome,
            };
            envia_com_ack(soquete, &aviso_premio, TIMEOUT_MS, MAX_TENTATIVAS);
            transfer_envia_arquivo(soquete, caminho, &seq_envio, TIMEOUT_MS, MAX_TENTATIVAS);
            log_info("Premio enviado.");
        }

        // Se coletou todas as pastilhas: vitória (nem precisa mover os fantasmas)
        if (jogo_venceu(&jogo)) {
            const char *texto = "Voce venceu!";
            PacmanPacket fim = {
                .tamanho = (unsigned char)strlen(texto),
                .sequencia = seq_envio,
                .tipo = MSG_FIM_TRANS,
                .dados = (unsigned char *)texto,
            };
            envia_com_ack(soquete, &fim, TIMEOUT_MS, MAX_TENTATIVAS);
            log_enviado(&fim);
            log_info("Fim de jogo: vitoria (todas as pastilhas coletadas).");
            break;
        }

        move_fantasmas(&jogo);

        // Se algum fantasma chegou a uma das 8 casas adjacentes ao pacman: derrota
        if (jogo_perdeu(&jogo)) {
            const char *nome = "game_over.jpg";
            PacmanPacket aviso_game_over = {
                .tamanho = (unsigned char)strlen(nome),
                .sequencia = seq_envio,
                .tipo = MSG_JPG,
                .dados = (unsigned char *)nome,
            };
            envia_com_ack(soquete, &aviso_game_over, TIMEOUT_MS, MAX_TENTATIVAS);
            transfer_envia_arquivo(soquete, IMAGEM_GAME_OVER, &seq_envio, TIMEOUT_MS, MAX_TENTATIVAS);

            const char *texto = "Fim de jogo!";
            PacmanPacket fim = {
                .tamanho = (unsigned char)strlen(texto),
                .sequencia = seq_envio,
                .tipo = MSG_FIM_TRANS,
                .dados = (unsigned char *)texto,
            };
            envia_com_ack(soquete, &fim, TIMEOUT_MS, MAX_TENTATIVAS);
            log_info("Fim de jogo: derrota (um fantasma alcancou o pacman).");
            break;
        }

        // Envia a nova visualização
        jogo_monta_janela(&jogo, janela, &total_vis);
        transfer_envia_visualizacao(soquete, janela, total_vis, &seq_envio, TIMEOUT_MS, MAX_TENTATIVAS);
        log_info("Visualizacao enviada.");
    }

    close(soquete);
    return 0;
}
