#ifndef TRANSFER_H
#define TRANSFER_H

#include "protocol.h"

#define TRANSFER_TAM_PACOTE 31 // tamanho tem 5 bits

int transfer_envia_visualizacao(int soquete, const unsigned char *dados, int total,
                                unsigned char *seq, int timeoutMillis, int max_tentativas);

int transfer_recebe_dados(int soquete, unsigned char *dados, unsigned char *tipo_final,
                          int timeoutMillis, int max_tentativas);

int transfer_envia_arquivo(int soquete, const char *caminho, unsigned char *seq,
                           int timeoutMillis, int max_tentativas);

int transfer_recebe_arquivo(int soquete, const char *caminho,
                            int timeoutMillis, int max_tentativas);

#endif
