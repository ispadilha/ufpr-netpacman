#ifndef PROTOCOL_H
#define PROTOCOL_H

// Marcador de Início definido no protocolo (em binário é 01111110, igual no Kermit)
#define START_MARKER 0x7E

typedef enum
{
    MSG_ACK = 0,
    MSG_NACK = 1,
    MSG_VISUALIZACAO = 2,
    MSG_INIT = 3,
    MSG_DADOS = 4,
    MSG_TXT = 5,
    MSG_JPG = 6,
    MSG_MP4 = 7,
    MSG_DIREITA = 10,
    MSG_ESQUERDA = 11,
    MSG_CIMA = 12,
    MSG_BAIXO = 13,
    MSG_ERRO = 15,
    MSG_FIM_TRANS = 16
} MessageType;

typedef struct
{
    unsigned char tamanho;
    unsigned char sequencia;
    unsigned char tipo;
    unsigned char *dados;
    unsigned char crc;
} PacmanPacket;

// Códigos de retorno pro deserialize_packet
#define PKT_OK 0
#define PKT_INVALIDO -1
#define PKT_CRC_ERRO -2

// Funções para transformar a Struct em um array de bytes e vice-versa
int serialize_packet(const PacmanPacket *pkt, unsigned char *buffer);
int deserialize_packet(const unsigned char *buffer, int length, PacmanPacket *pkt);

#endif
