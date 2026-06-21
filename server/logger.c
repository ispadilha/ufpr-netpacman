#include <stdio.h>
#include <time.h>
#include "logger.h"

static const char *tipo_nome(unsigned char tipo)
{
    switch (tipo)
    {
    case MSG_ACK:
        return "ACK";
    case MSG_NACK:
        return "NACK";
    case MSG_VISUALIZACAO:
        return "VISUALIZACAO";
    case MSG_INIT:
        return "INIT";
    case MSG_DADOS:
        return "DADOS";
    case MSG_TXT:
        return "TXT";
    case MSG_JPG:
        return "JPG";
    case MSG_MP4:
        return "MP4";
    case MSG_DIREITA:
        return "DIREITA";
    case MSG_ESQUERDA:
        return "ESQUERDA";
    case MSG_CIMA:
        return "CIMA";
    case MSG_BAIXO:
        return "BAIXO";
    case MSG_ERRO:
        return "ERRO";
    case MSG_FIM_TRANS:
        return "FIM_TRANS";
    default:
        return "?";
    }
}

void log_recebido(const PacmanPacket *pkt)
{
    printf("[R] tipo=%-13s seq=%-2u tam=%u\n", tipo_nome(pkt->tipo), pkt->sequencia, pkt->tamanho);
}

void log_enviado(const PacmanPacket *pkt)
{
    printf("[T] tipo=%-13s seq=%-2u tam=%u\n", tipo_nome(pkt->tipo), pkt->sequencia, pkt->tamanho);
}

void log_info(const char *msg)
{
    printf("[I] %s\n", msg);
}

void log_erro(const char *msg)
{
    printf("[E] %s\n", msg);
}