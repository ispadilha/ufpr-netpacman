#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "map.h"

int carrega_mapa(const char *caminho, char mapa[MAP_ROWS][MAP_COLS])
{
    FILE *f = fopen(caminho, "r");
    if (!f) return -1;

    int row = 0;
    char linha[256];
    while (row < MAP_ROWS && fgets(linha, sizeof(linha), f))
    {
        int col = 0;
        for (int i = 0; linha[i] && col < MAP_COLS; i++)
        {
            if (linha[i] == ';' || linha[i] == '\n' || linha[i] == '\r')
                continue;
            mapa[row][col++] = linha[i];
        }
        row++;
    }
    fclose(f);
    return (row == MAP_ROWS) ? 0 : -1;
}

int posiciona_caractere(char mapa[MAP_ROWS][MAP_COLS], char caractere, int *out_linha, int *out_coluna)
{
    int vazias = 0;
    for (int r = 0; r < MAP_ROWS; r++)
        for (int c = 0; c < MAP_COLS; c++)
            if (mapa[r][c] == '0') vazias++;
    if (vazias == 0) return -1;

    srand(time(NULL));

    int alvo = rand() % vazias;
    int contador = 0;
    for (int r = 0; r < MAP_ROWS; r++)
        for (int c = 0; c < MAP_COLS; c++)
            if (mapa[r][c] == '0') {
                if (contador == alvo) {
                    mapa[r][c] = caractere;
                    if (out_linha)
                        *out_linha = r;
                    if (out_coluna)
                        *out_coluna = c;
                    return 0;
                }
                contador++;
            }
    return -1;
}