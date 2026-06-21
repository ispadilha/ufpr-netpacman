#ifndef SOCKET_H
#define SOCKET_H

#include "protocol.h"

// Cria e configura o Raw Socket no modo promíscuo
int cria_raw_socket(char* nome_interface_rede);

int envia_mensagem(int soquete, const PacmanPacket *pacote);

int recebe_mensagem(int soquete, int timeoutMillis, PacmanPacket *pacote_recebido);

// Envia um pacote e aguarda o ACK, retransmitindo em caso de timeout ou NACK
// Retorna 0 se o ACK chegou, ou -1 se esgotou as tentativas
int envia_com_ack(int soquete, const PacmanPacket *pacote, int timeoutMillis, int max_tentativas);

#endif
