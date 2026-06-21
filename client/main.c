#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../shared/socket.h"
#include "../shared/protocol.h"
#include "../shared/screen.h"
#include "view.h"
#include "input.h"

#define TIMEOUT_MS 1000
#define MAX_TENTATIVAS 30

static void desenha(const PacmanPacket *vis, int lado)
{
    clearScreen();
    exibe_janela((const char *)vis->dados, vis->tamanho, lado);
    printf("\nUse WASD para mover. Esc para sair.\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Uso: %s <interface>\n", argv[0]);
        return 1;
    }

    int soquete = cria_raw_socket(argv[1]);

    // Envia o INIT até receber o ACK (o servidor pode subir depois do cliente)
    PacmanPacket init = {.tamanho = 0, .sequencia = 0, .tipo = MSG_INIT};
    if (envia_com_ack(soquete, &init, TIMEOUT_MS, MAX_TENTATIVAS) != 0)
    {
        printf("Servidor não respondeu. Encerrando.\n");
        close(soquete);
        return 1;
    }

    // Recebe a visualização inicial e confirma (manda um ACK)
    PacmanPacket vis;
    if (recebe_mensagem(soquete, 3000, &vis) < 0 || vis.tipo != MSG_VISUALIZACAO)
    {
        printf("Erro ao receber a visualização do jogo.\n");
        close(soquete);
        return 1;
    }
    PacmanPacket ack_vis = {.tamanho = 0, .sequencia = vis.sequencia, .tipo = MSG_ACK};
    envia_mensagem(soquete, &ack_vis);

    int movimentos = 0;
    desenha(&vis, janela_lado(movimentos));

    input_inicia();
    unsigned char seq = 0;
    int jogando = 1;

    // Loop do jogo: lê comando, envia pro servidor e mostra a resposta
    while (jogando)
    {
        int tecla = le_tecla();
        if (tecla == INPUT_SAIR)
        {
            // Avisa o servidor para encerrar também, antes de sair
            PacmanPacket fim = {.tamanho = 0, .sequencia = seq, .tipo = MSG_FIM_TRANS};
            envia_com_ack(soquete, &fim, TIMEOUT_MS, MAX_TENTATIVAS);
            break;
        }
        if (tecla == INPUT_IGNORAR)
            continue;

        // Envia o movimento e espera o ACK
        PacmanPacket mov = {.tamanho = 0, .sequencia = seq, .tipo = (unsigned char)tecla};
        if (envia_com_ack(soquete, &mov, TIMEOUT_MS, MAX_TENTATIVAS) != 0)
        {
            printf("\nServidor não respondeu. Encerrando.\n");
            break;
        }
        seq = (seq + 1) % 64; // sequencia tem 6 bits: volta a 0 após 63
        movimentos++;

        // Espera a resposta do servidor: nova visualização ou fim de jogo
        PacmanPacket resp;
        int recebeu = 0;
        for (int t = 0; t < MAX_TENTATIVAS && !recebeu; t++)
        {
            if (recebe_mensagem(soquete, TIMEOUT_MS, &resp) > 0 &&
                (resp.tipo == MSG_VISUALIZACAO || resp.tipo == MSG_FIM_TRANS))
            {
                recebeu = 1;
            }
            else
            {
                // Se nada chegou, cutuca o servidor reenviando o movimento
                // (ele deduplica pela sequência e reenvia a última visualização)
                envia_com_ack(soquete, &mov, TIMEOUT_MS, 3);
            }
        }
        if (!recebeu)
        {
            printf("\nSem resposta do servidor. Encerrando.\n");
            break;
        }

        // Confirma o recebimento antes de qualquer coisa
        PacmanPacket ack = {.tamanho = 0, .sequencia = resp.sequencia, .tipo = MSG_ACK};
        envia_mensagem(soquete, &ack);

        if (resp.tipo == MSG_FIM_TRANS) // fim de jogo
        {
            clearScreen();
            char msg[64];
            int n = resp.tamanho < sizeof(msg) - 1 ? resp.tamanho : sizeof(msg) - 1;
            memcpy(msg, resp.dados, n);
            msg[n] = '\0';
            printf("%s\n", msg);
            jogando = 0;
        }
        else
        { // MSG_VISUALIZACAO
            desenha(&resp, janela_lado(movimentos));
        }
    }

    input_restaura();
    close(soquete);
    return 0;
}
