#include "protocol.h"
#include "crc.h"

static void escreve_bit(unsigned char *buffer, int pos_bit, int valor)
{
    int byte_index = pos_bit / 8;
    int bit_offset = 7 - (pos_bit % 8); // bit mais significativo primeiro

    if (valor)
        // 1
        buffer[byte_index] |= (1 << bit_offset);
    else
        // 0
        buffer[byte_index] &= ~(1 << bit_offset);
}

static int le_bit(const unsigned char *buffer, int pos_bit)
{
    int byte_index = pos_bit / 8;
    int bit_offset = 7 - (pos_bit % 8); // bit mais significativo primeiro
    return (buffer[byte_index] >> bit_offset) & 1;
}

static void escreve_campo(unsigned char *buffer, int *pos_bit, unsigned int valor, int n_bits)
{
    for (int b = n_bits - 1; b >= 0; b--)
        escreve_bit(buffer, (*pos_bit)++, (valor >> b) & 1);
}

static unsigned int le_campo(const unsigned char *buffer, int *pos_bit, int n_bits)
{
    unsigned int valor = 0;
    for (int b = n_bits - 1; b >= 0; b--)
        valor |= (unsigned int)le_bit(buffer, (*pos_bit)++) << b;
    return valor;
}

int serialize_packet(const PacmanPacket *pkt, unsigned char *buffer)
{
    if (!pkt || !buffer)
        return -1;

    // Verifica se os campos estão corretos de acordo com o protocolo
    if (pkt->tamanho > 31 || pkt->sequencia > 63 || pkt->tipo > 31)
        return -1;

    int pos_bit = 0;

    escreve_campo(buffer, &pos_bit, START_MARKER, 8);
    escreve_campo(buffer, &pos_bit, pkt->tamanho, 5);
    escreve_campo(buffer, &pos_bit, pkt->sequencia, 6);
    escreve_campo(buffer, &pos_bit, pkt->tipo, 5);

    // Dados: pkt->tamanho bytes (8 bits cada)
    for (int i = 0; i < pkt->tamanho; i++)
        escreve_campo(buffer, &pos_bit, pkt->dados[i], 8);

    // CRC: calculado sobre todos os bits escritos até agora
    /* Como escrevemos exatamente campos de 8+5+6+5 = 24 bits (3 bytes) + dados (múltiplo de 8),
    pos_bit sempre será múltiplo de 8 */
    int bytes_antes_crc = pos_bit / 8;
    unsigned char crc = calc_crc8(buffer, bytes_antes_crc);
    escreve_campo(buffer, &pos_bit, crc, 8);

    return pos_bit / 8; // número de bytes serializados
}

int deserialize_packet(const unsigned char *buffer, int length, PacmanPacket *pkt)
{
    if (!buffer || !pkt || length < 4) // mínimo: 3 bytes cabeçalho + 1 CRC = 4
        return PKT_INVALIDO;

    int pos_bit = 0;

    // Lê e valida o marcador (8 bits)
    unsigned char marcador = le_campo(buffer, &pos_bit, 8);
    if (marcador != START_MARKER)
        return PKT_INVALIDO;

    // Lê o tamanho (5 bits) e valida se o frame recebido comporta os dados
    pkt->tamanho = le_campo(buffer, &pos_bit, 5);
    if (length < 4 + pkt->tamanho) // 3 bytes de cabeçalho + tamanho + 1 de CRC, portanto 4 + tamanho
        return PKT_INVALIDO;

    pkt->sequencia = le_campo(buffer, &pos_bit, 6);
    pkt->tipo = le_campo(buffer, &pos_bit, 5);

    // Agora pos_bit está no início dos dados (que ocupam pkt->tamanho bytes)

    // Para os dados, aponta para dentro do buffer original
    // O chamador (recebe_mensagem) garante que o buffer sobreviva até o uso
    int byte_offset_dados = pos_bit / 8;
    pkt->dados = (unsigned char *)&buffer[byte_offset_dados];

    // Avança o pos_bit sobre os dados
    int bits_dados = pkt->tamanho * 8;
    pos_bit += bits_dados;

    // Lê o CRC do emissor pra comparar com o que for calculado
    unsigned char crc_recebido = le_campo(buffer, &pos_bit, 8);

    // Usa os bytes anteriores pra calcular/verificar o CRC
    int bytes_antes_crc = (pos_bit - 8) / 8; // posição atual menos os 8 bits do CRC
    unsigned char crc_calc = calc_crc8(buffer, bytes_antes_crc);
    if (crc_calc != crc_recebido)
        return PKT_CRC_ERRO;

    pkt->crc = crc_recebido;
    return PKT_OK;
}
