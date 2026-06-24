#ifndef MOVEMENTS_H
#define MOVEMENTS_H

#include "game.h"

// Retorna a pastilha se coletou alguma (1 a 6) ou 0 se não
int move_pacman(GameState *jogo, Direcao dir);
void move_fantasmas(GameState *jogo);

// Converte o tipo da mensagem (MSG_*) numa Direcao
Direcao tipo_para_direcao(unsigned char tipo);

#endif
