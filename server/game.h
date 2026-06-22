#ifndef GAME_H
#define GAME_H

#include "map.h"

#define NUM_FANTASMAS 4
#define NUM_PASTILHAS 6

typedef enum { CIMA = 0, BAIXO = 1, ESQUERDA = 2, DIREITA = 3 } Direcao;

typedef struct {
    int r, c;
    Direcao dir;
    char simbolo;
    int alterna_verde; // usado só pelo verde pra alternar direção
} Fantasma;

typedef struct {
    char base[MAP_ROWS][MAP_COLS]; // "camada estática": paredes, vazios, pastilhas
    int pr, pc;
    Fantasma fantasmas[NUM_FANTASMAS];
    int coletadas[NUM_PASTILHAS + 1]; // coletadas[d] == 1 se "d" foi coletada
    int total_coletadas;
    int movimentos; // nº de movimentos processados pra aumentar a visualização
} GameState;

int jogo_inicializa(const char *caminho_mapa, GameState *jogo);

int jogo_venceu(const GameState *jogo); // coletou todas as pastilhas
int jogo_perdeu(const GameState *jogo); // fantasma em casa vizinha do pacman

void jogo_monta_janela(const GameState *jogo, unsigned char *janela, int *tamanho);

#endif
