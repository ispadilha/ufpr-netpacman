#include <stdio.h>
#include <string.h>
#include "game.h"
#include "../shared/screen.h"

// Distância de Chebyshev: vale 1 para as 8 casas vizinhas
static int chebyshev(int r1, int c1, int r2, int c2)
{
    int dr = (r1 > r2) ? r1 - r2 : r2 - r1;
    int dc = (c1 > c2) ? c1 - c2 : c2 - c1;
    return (dr > dc) ? dr : dc;
}

int jogo_inicializa(const char *caminho_mapa, GameState *jogo)
{
    char mapa_base[MAP_ROWS][MAP_COLS];
    if (carrega_mapa(caminho_mapa, mapa_base) != 0)
        return -1;

    const char simbolos_fantasmas[NUM_FANTASMAS] = {'R', 'B', 'G', 'Y'};
    const int MAX_TENTATIVAS = 50;

    for (int tentativa = 0; tentativa < MAX_TENTATIVAS; tentativa++)
    {
        char mapa[MAP_ROWS][MAP_COLS];
        memcpy(mapa, mapa_base, sizeof(mapa));

        int ok = 1;

        // Sorteia posições para pastilhas
        for (char d = '1'; d <= '6'; d++)
            if (posiciona_caractere(mapa, d, NULL, NULL) != 0)
                ok = 0;

        // Sorteia posições para pacman e fantasmas
        int pr = 0, pc = 0;
        int fr[NUM_FANTASMAS], fc[NUM_FANTASMAS];
        if (posiciona_caractere(mapa, 'P', &pr, &pc) != 0)
            ok = 0;
        for (int i = 0; i < NUM_FANTASMAS; i++)
            if (posiciona_caractere(mapa, simbolos_fantasmas[i], &fr[i], &fc[i]) != 0)
                ok = 0;

        if (!ok)
            continue;

        // Se algum fantasma nasceu nas 8 casas vizinhas do pacman, gera de novo
        int adjacente = 0;
        for (int i = 0; i < NUM_FANTASMAS; i++)
        {
            if (chebyshev(pr, pc, fr[i], fc[i]) <= 1)
                adjacente = 1;
        }
        if (adjacente)
            continue;

        // Aqui precisa "limpar" os caracteres dos personagens "móveis"
        // pra colocar o mapa-base ("camada estática") no estado do jogo (GameState)
        mapa[pr][pc] = '0';
        for (int i = 0; i < NUM_FANTASMAS; i++)
            mapa[fr[i]][fc[i]] = '0';

        memcpy(jogo->base, mapa, sizeof(mapa));

        // Agora sim posiciona os personagens no GameState
        jogo->pr = pr;
        jogo->pc = pc;
        for (int i = 0; i < NUM_FANTASMAS; i++)
        {
            jogo->fantasmas[i].r = fr[i];
            jogo->fantasmas[i].c = fc[i];
            jogo->fantasmas[i].simbolo = simbolos_fantasmas[i];
            jogo->fantasmas[i].alterna_verde = 1;
            jogo->fantasmas[i].dir = BAIXO;
        }

        // E termina de inicializar o GameState
        for (int d = 0; d <= NUM_PASTILHAS; d++)
            jogo->coletadas[d] = 0;
        jogo->total_coletadas = 0;
        jogo->movimentos = 0;

        return 0;
    }

    return -2; // apenas um fallback, é difícil acontecer eu acho (depende do mapa)
}

int jogo_venceu(const GameState *jogo)
{
    return jogo->total_coletadas >= NUM_PASTILHAS;
}

int jogo_perdeu(const GameState *jogo)
{
    for (int i = 0; i < NUM_FANTASMAS; i++)
        if (chebyshev(jogo->pr, jogo->pc, jogo->fantasmas[i].r, jogo->fantasmas[i].c) <= 1)
            return 1;
    return 0;
}

// Compõe o tabuleiro completo: camada estática + fantasmas + pacman por cima
static void compoe_mapa(const GameState *jogo, char saida[MAP_ROWS][MAP_COLS])
{
    memcpy(saida, jogo->base, sizeof(jogo->base));
    for (int i = 0; i < NUM_FANTASMAS; i++)
        saida[jogo->fantasmas[i].r][jogo->fantasmas[i].c] = jogo->fantasmas[i].simbolo;
    saida[jogo->pr][jogo->pc] = 'P';
}

void jogo_monta_janela(const GameState *jogo, unsigned char *janela, unsigned char *tamanho)
{
    char mapa[MAP_ROWS][MAP_COLS];
    compoe_mapa(jogo, mapa);

    int raio = janela_raio(jogo->movimentos);
    int k = 0;
    for (int dr = -raio; dr <= raio; dr++)
    {
        for (int dc = -raio; dc <= raio; dc++)
        {
            int r = jogo->pr + dr;
            int c = jogo->pc + dc;
            if (r < 0 || r >= MAP_ROWS || c < 0 || c >= MAP_COLS)
                janela[k++] = 'X'; // fora do mapa = parede
            else
                janela[k++] = (unsigned char)mapa[r][c];
        }
    }
    *tamanho = (unsigned char)k;
}

void jogo_renderiza_servidor(const GameState *jogo)
{
    char mapa[MAP_ROWS][MAP_COLS];
    compoe_mapa(jogo, mapa);

    clearScreen();
    imprime_mapa(mapa);
    printf("Pastilhas: %d/%d   Movimentos: %d\n", jogo->total_coletadas, NUM_PASTILHAS, jogo->movimentos);
}
