#include <stdlib.h>
#include "movements.h"
#include "../shared/protocol.h"

// Vetores de deslocamento por direção (CIMA, BAIXO, ESQUERDA, DIREITA)
static const int DR[4] = { -1, +1,  0,  0 };
static const int DC[4] = {  0,  0, -1, +1 };

static Direcao vira_esquerda(Direcao d)
{
    switch (d) {
        case CIMA:     return ESQUERDA;
        case ESQUERDA: return BAIXO;
        case BAIXO:    return DIREITA;
        case DIREITA:  return CIMA;
    }
    return d;
}

static Direcao vira_direita(Direcao d)
{
    switch (d) {
        case CIMA:     return DIREITA;
        case DIREITA:  return BAIXO;
        case BAIXO:    return ESQUERDA;
        case ESQUERDA: return CIMA;
    }
    return d;
}

Direcao tipo_para_direcao(unsigned char tipo)
{
    switch (tipo) {
        case MSG_CIMA:     return CIMA;
        case MSG_BAIXO:    return BAIXO;
        case MSG_ESQUERDA: return ESQUERDA;
        case MSG_DIREITA:  return DIREITA;
        default:           return DIREITA;
    }
}

static int eh_parede(const GameState *jogo, int r, int c)
{
    if (r < 0 || r >= MAP_ROWS || c < 0 || c >= MAP_COLS)
        return 1; // fora do mapa = parede
    return jogo->base[r][c] == 'X';
}

// Verifica se a célula (r, c) está ocupada pelo pacman ou por algum fantasma
// índice do fantasma verificando é ignorado, pra não verificar ele mesmo como obstáculo
static int ocupada_por_entidade(const GameState *jogo, int r, int c, int indice_ignorado)
{
    if (r == jogo->pr && c == jogo->pc)
        return 1;
    for (int i = 0; i < NUM_FANTASMAS; i++) {
        if (i == indice_ignorado)
            continue;
        if (jogo->fantasmas[i].r == r && jogo->fantasmas[i].c == c)
            return 1;
    }
    return 0;
}

// Um fantasma só entra numa célula que não seja parede nem esteja ocupada
static int livre_para_fantasma(const GameState *jogo, int indice_fantasma, Direcao d)
{
    int nr = jogo->fantasmas[indice_fantasma].r + DR[d];
    int nc = jogo->fantasmas[indice_fantasma].c + DC[d];
    if (eh_parede(jogo, nr, nc)) return 0;
    if (ocupada_por_entidade(jogo, nr, nc, indice_fantasma))
        return 0;
    return 1;
}

void move_pacman(GameState *jogo, Direcao dir)
{
    int nr = jogo->pr + DR[dir];
    int nc = jogo->pc + DC[dir];

    if (eh_parede(jogo, nr, nc))
        return; // não atravessa parede (o turno passa mesmo assim)

    // Coleta de pastilha
    char alvo = jogo->base[nr][nc];
    if (alvo >= '1' && alvo <= '6') {
        int d = alvo - '0';
        if (!jogo->coletadas[d]) {
            jogo->coletadas[d] = 1;
            jogo->total_coletadas++;
        }
        jogo->base[nr][nc] = '0'; // pastilha consumida
    }

    jogo->pr = nr;
    jogo->pc = nc;
}

static void move_um_fantasma(GameState *jogo, int indice_fantasma)
{
    Fantasma *f = &jogo->fantasmas[indice_fantasma];

    // Amarelo: anda numa direção livre aleatória
    if (f->simbolo == 'Y') {
        Direcao livres[4];
        int n = 0;
        for (int d = 0; d < 4; d++)
            if (livre_para_fantasma(jogo, indice_fantasma, (Direcao)d))
                livres[n++] = (Direcao)d;
        if (n == 0) return; // encurralado
        Direcao d = livres[rand() % n];
        f->dir = d;
        f->r += DR[d];
        f->c += DC[d];
        return;
    }

    // Vermelho, azul e verde: seguem em frente até bater numa parede
    if (livre_para_fantasma(jogo, indice_fantasma, f->dir))
    {
        f->r += DR[f->dir];
        f->c += DC[f->dir];
        return;
    }

    // Bateu: escolhe o sentido da virada
    Direcao (*vira)(Direcao);
    if (f->simbolo == 'R') {
        vira = vira_esquerda;
    } else if (f->simbolo == 'B') {
        vira = vira_direita;
    }
    else
    { // 'G' (verde): alterna
        vira = f->alterna_verde ? vira_direita : vira_esquerda;
        f->alterna_verde = !f->alterna_verde;
    }

    // Vira até encontrar uma direção livre
    Direcao d = f->dir;
    for (int i = 0; i < 4; i++) {
        d = vira(d);
        if (livre_para_fantasma(jogo, indice_fantasma, d))
        {
            f->dir = d;
            f->r += DR[d];
            f->c += DC[d];
            return;
        }
    }
}

void move_fantasmas(GameState *jogo)
{
    for (int i = 0; i < NUM_FANTASMAS; i++)
        move_um_fantasma(jogo, i);
}
