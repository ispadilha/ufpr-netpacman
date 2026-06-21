#ifndef SCREEN_H
#define SCREEN_H

// Limpa a tela do terminal (ANSI Escape Sequences)
void clearScreen(void);

// Imprime um caractere usando ANSI truecolor
void imprime_caractere_colorido(char c);

// Geometria da janela de visualização em função do nº de movimentos
int janela_raio(int movimentos);
int janela_lado(int movimentos);

#endif
