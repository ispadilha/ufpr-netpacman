#include <stdio.h>
#include "view.h"
#include "../shared/screen.h"

void exibe_janela(const char *bytes, int tamanho, int lado)
{
    for (int i = 0; i < tamanho; i++) {
        imprime_caractere_colorido(bytes[i]);
        // Quebra a linha a cada "lado" número de células
        if ((i + 1) % lado == 0)
            putchar('\n');
    }
}
