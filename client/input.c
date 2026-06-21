#include <unistd.h>
#include <termios.h>
#include "input.h"
#include "../shared/protocol.h"

// Guarda a configuração original do terminal pra restaurar na saída
static struct termios termios_original;

void input_inicia(void)
{
    // Lê a configuração atual do terminal (STDIN) e guarda em termios_original
    tcgetattr(STDIN_FILENO, &termios_original);

    // Cópia para modificar
    struct termios raw = termios_original;

    // Desliga duas "flags locais" (c_lflag) do terminal:
    // ICANON: modo canônico (por linha e esperando Enter)
    // ECHO: eco (o que se digita aparece na tela)
    raw.c_lflag &= ~(ICANON | ECHO);

    // Em modo não-canônico, VMIN e VTIME controlam quando read() retorna
    raw.c_cc[VMIN] = 1; // retorna após ler no mínimo 1 byte
    raw.c_cc[VTIME] = 0; // sem timeout, e com VMIN=1, read() bloqueia até vir uma tecla

    // Aplica a nova configuração imediatamente (TCSANOW = "agora")
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void input_restaura(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &termios_original);
}

static int le_byte(void)
{
    unsigned char b;
    return (read(STDIN_FILENO, &b, 1) == 1) ? b : -1;
}

int le_tecla(void)
{
    switch (le_byte())
    {
    case 'w':
    case 'W':
        return MSG_CIMA;
    case 's':
    case 'S':
        return MSG_BAIXO;
    case 'd':
    case 'D':
        return MSG_DIREITA;
    case 'a':
    case 'A':
        return MSG_ESQUERDA;
    case 0x1B: // Esc
        return INPUT_SAIR;
    default:
        return INPUT_IGNORAR;
    }
}
