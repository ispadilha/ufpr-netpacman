#ifndef INPUT_H
#define INPUT_H

// Valores especiais retornados por le_tecla() (além dos MSG_* das direções)
#define INPUT_SAIR -1
#define INPUT_IGNORAR 0

void input_inicia(void); // coloca o terminal em modo "raw" (sem enter, sem eco)
void input_restaura(void);

int le_tecla(void);

#endif
