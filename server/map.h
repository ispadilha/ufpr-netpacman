#ifndef MAP_H
#define MAP_H

#define MAP_ROWS 40
#define MAP_COLS 40

int carrega_mapa(const char *caminho, char mapa[MAP_ROWS][MAP_COLS]);
int posiciona_caractere(char mapa[MAP_ROWS][MAP_COLS], char caractere, int *out_linha, int *out_coluna);

#endif