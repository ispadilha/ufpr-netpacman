#include <stdio.h>
#include <string.h>
#include "transfer.h"
#include "socket.h"

int transfer_envia_visualizacao(int soquete, const unsigned char *dados, int total,
                                unsigned char *seq, int timeoutMillis, int max_tentativas)
{
    int enviados = 0;

    do // do-while pq precisa enviar pelo menos um
    {
        int restante = total - enviados;
        int n = restante > TRANSFER_TAM_PACOTE ? TRANSFER_TAM_PACOTE : restante;
        int eh_ultimo = (enviados + n >= total);

        PacmanPacket pkt = {
            .tamanho = (unsigned char)n,
            .sequencia = *seq,
            .tipo = eh_ultimo ? MSG_VISUALIZACAO : MSG_DADOS, // ver o último comentário ali embaixo
            .dados = (unsigned char *)&dados[enviados],
        };

        if (envia_com_ack(soquete, &pkt, timeoutMillis, max_tentativas) != 0)
            return -1;

        enviados += n;
        *seq = (*seq + 1) % 64; // sequencia tem 6 bits: volta a 0 após 63
    } while (enviados < total);

    return 0;
}

int transfer_recebe_visualizacao(int soquete, unsigned char *dados, unsigned char *tipo_final,
                                 int timeoutMillis, int max_tentativas)
{
    int total = 0;
    int recebeu_algum = 0;
    unsigned char seq_esperada = 0;

    for (int tentativas = 0; tentativas < max_tentativas;)
    {
        PacmanPacket pkt;
        if (recebe_mensagem(soquete, timeoutMillis, &pkt) < 0)
        {
            tentativas++;
            continue;
        }

        if (pkt.tipo != MSG_DADOS && pkt.tipo != MSG_VISUALIZACAO &&
            pkt.tipo != MSG_FIM_TRANS && pkt.tipo != MSG_JPG)
            continue;

        // Primeiro pacote
        if (!recebeu_algum)
            seq_esperada = pkt.sequencia;

        // Se for o pedaço esperado, monta nos dados
        if (pkt.sequencia == seq_esperada)
        {
            memcpy(&dados[total], pkt.dados, pkt.tamanho);
            total += pkt.tamanho;
            seq_esperada = (seq_esperada + 1) % 64;
            recebeu_algum = 1;
        }

        // Confirma o pedaço (manda um ACK)
        PacmanPacket ack = {.tamanho = 0, .sequencia = pkt.sequencia, .tipo = MSG_ACK};
        envia_mensagem(soquete, &ack);

        // MSG_DADOS são "pedaços do meio" das transferências, os outros encerram
        if (pkt.tipo == MSG_VISUALIZACAO || pkt.tipo == MSG_FIM_TRANS || pkt.tipo == MSG_JPG)
        {
            *tipo_final = pkt.tipo;
            return total;
        }

        tentativas = 0;
    }

    return -1;
}

int transfer_envia_arquivo(int soquete, const char *caminho, unsigned char *seq,
                           int timeoutMillis, int max_tentativas)
{
    FILE *arquivo = fopen(caminho, "rb");
    if (arquivo == NULL)
        return -1;

    unsigned char buffer[TRANSFER_TAM_PACOTE];
    int n;

    while ((n = fread(buffer, 1, TRANSFER_TAM_PACOTE, arquivo)) > 0)
    {
        PacmanPacket pkt = {
            .tamanho = (unsigned char)n,
            .sequencia = *seq,
            .tipo = MSG_DADOS,
            .dados = buffer,
        };
        if (envia_com_ack(soquete, &pkt, timeoutMillis, max_tentativas) != 0)
        {
            fclose(arquivo);
            return -1;
        }
        *seq = (*seq + 1) % 64;
    }
    fclose(arquivo);

    PacmanPacket fim = {
        .tamanho = 0,
        .sequencia = *seq,
        .tipo = MSG_FIM_TRANS,
        .dados = buffer,
    };
    if (envia_com_ack(soquete, &fim, timeoutMillis, max_tentativas) != 0)
        return -1;
    *seq = (*seq + 1) % 64;

    return 0;
}

int transfer_recebe_arquivo(int soquete, const char *caminho,
                            int timeoutMillis, int max_tentativas)
{
    FILE *arquivo = fopen(caminho, "wb");
    if (arquivo == NULL)
        return -1;

    int recebeu_algum = 0;
    unsigned char seq_esperada = 0;

    for (int tentativas = 0; tentativas < max_tentativas;)
    {
        PacmanPacket pkt;
        if (recebe_mensagem(soquete, timeoutMillis, &pkt) < 0)
        {
            tentativas++;
            continue;
        }

        if (pkt.tipo != MSG_DADOS && pkt.tipo != MSG_FIM_TRANS)
            continue;

        // Primeiro pacote
        if (!recebeu_algum)
            seq_esperada = pkt.sequencia;

        // Se for o pedaço esperado, escreve
        if (pkt.sequencia == seq_esperada)
        {
            fwrite(pkt.dados, 1, pkt.tamanho, arquivo);
            seq_esperada = (seq_esperada + 1) % 64;
            recebeu_algum = 1;
        }

        // Confirma o pedaço (manda um ACK)
        PacmanPacket ack = {.tamanho = 0, .sequencia = pkt.sequencia, .tipo = MSG_ACK};
        envia_mensagem(soquete, &ack);

        if (pkt.tipo == MSG_FIM_TRANS)
        {
            fclose(arquivo);
            return 0;
        }

        tentativas = 0;
    }

    fclose(arquivo);
    return -1;
}
