#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../shared/socket.h"
#include "../shared/protocol.h"
#include "../shared/screen.h"
#include "../shared/transfer.h"
#include "view.h"
#include "input.h"

#define TIMEOUT_MS 1000
#define MAX_TENTATIVAS 30

static void desenha(const unsigned char *bytes, int total, int lado)
{
    clearScreen();
    exibe_janela((const char *)bytes, total, lado);
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

    // Recebe a visualização inicial
    unsigned char janela[MAX_JANELA];
    unsigned char tipo_final;
    int total = transfer_recebe_visualizacao(soquete, janela, &tipo_final, 3000, MAX_TENTATIVAS);
    if (total < 0 || tipo_final != MSG_VISUALIZACAO)
    {
        printf("Erro ao receber a visualização do jogo.\n");
        close(soquete);
        return 1;
    }

    int movimentos = 0;
    desenha(janela, total, janela_lado(movimentos));

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
        int total_resp = -1;
        for (int t = 0; t < MAX_TENTATIVAS && total_resp < 0; t++)
        {
            total_resp = transfer_recebe_visualizacao(soquete, janela, &tipo_final, TIMEOUT_MS, MAX_TENTATIVAS);
            if (total_resp < 0)
            {
                // Se nada chegou, cutuca o servidor reenviando o movimento
                // (ele deduplica pela sequência e reenvia a última visualização)
                envia_com_ack(soquete, &mov, TIMEOUT_MS, 3);
            }
        }
        if (total_resp < 0)
        {
            printf("\nSem resposta do servidor. Encerrando.\n");
            break;
        }

        if (tipo_final == MSG_FIM_TRANS) // fim de jogo
        {
            clearScreen();
            char msg[64];
            int n = total_resp < (int)sizeof(msg) - 1 ? total_resp : (int)sizeof(msg) - 1;
            memcpy(msg, janela, n);
            msg[n] = '\0';
            printf("%s\n", msg);
            jogando = 0;
        }
        else
        { // MSG_VISUALIZACAO
            desenha(janela, total_resp, janela_lado(movimentos));
        }
    }

    input_restaura();
    close(soquete);
    return 0;
}
