#include "crc.h"

/* Polinômio gerador para CRC-8 (x^8 + x^2 + x + 1)
Existem vários, este é conhecido como CRC-8-ATM ou CRC-8-CCITT,
e é uma escolha comum e confiável para pacotes pequenos */
#define CRC8_POLY 0x07

unsigned char calc_crc8(const unsigned char *data, int len) {
    unsigned char crc = 0x00;

    for (int i = 0; i < len; i++) {
        crc ^= data[i]; // Faz o XOR do byte atual com o CRC
        
        // Processa cada um dos 8 bits do byte
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                // Se o bit mais significativo for 1, desloca e faz XOR com o polinômio
                crc = (crc << 1) ^ CRC8_POLY;
            } else {
                // Senão, apenas desloca
                crc <<= 1;
            }
        }
    }
    
    return crc;
}
