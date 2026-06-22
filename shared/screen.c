#include <stdio.h>
#include "screen.h"

// Cores ANSI 24-bit (truecolor), no formato "\033[38;2;R;G;Bm"
#define COR_RESET "\033[0m"
#define COR_PAREDE "\033[38;2;255;255;255m"
#define COR_VAZIO "\033[38;2;80;80;80m"
#define COR_VERMELHO "\033[38;2;255;40;40m"
#define COR_VERDE "\033[38;2;40;220;40m"
#define COR_AZUL "\033[38;2;60;130;255m"
#define COR_AMARELO "\033[38;2;240;220;40m"
#define COR_PASTILHA "\033[38;2;170;80;230m"
#define COR_PACMAN "\033[38;2;255;110;180m"

// Limpa a tela conforme pesquisa sobre "ANSI Escape Sequences"
void clearScreen(void)
{
    printf("\033[1;1H\033[2J");
}

void imprime_caractere_colorido(char c)
{
    const char *cor;
    switch (c) {
        case 'X': cor = COR_PAREDE;   break;
        case '0': cor = COR_VAZIO;    break;
        case 'R': cor = COR_VERMELHO; break;
        case 'G': cor = COR_VERDE;    break;
        case 'B': cor = COR_AZUL;     break;
        case 'Y': cor = COR_AMARELO;  break;
        case 'P': cor = COR_PACMAN;   break;
        default:
            cor = (c >= '1' && c <= '6') ? COR_PASTILHA : COR_RESET;
            break;
    }
    printf("%s%c" COR_RESET, cor, c);
}

int janela_raio(int movimentos)
{
    int raio = 1 + movimentos / 5;
    if (raio > MAX_RAIO)
        raio = MAX_RAIO;
    return raio;
}

int janela_lado(int movimentos)
{
    return 2 * janela_raio(movimentos) + 1;
}
