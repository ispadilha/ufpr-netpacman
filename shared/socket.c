#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <sys/time.h>

#include "socket.h"
#include "protocol.h"

#define FRAME_MAX 35
/*
Marcador (8 bits)
Tamanho (5 bits)
Sequência (6 bits)
Tipo (5 bits)
Dados (0..31 bytes = 0..248 bits)
CRC (8 bits)
Total: 3 bytes de cabeçalho + dados + 1 byte de CRC = 4..35 bytes
*/

#define PADDING_LEN 14
/*
As placas de rede "reclamaram" de argumento inválido no send ao testar,
colocando um padding de 14 bytes (que seria o tamanho de um cabeçalho Ethernet) resolveu
*/

#define BUFFER_MAX (FRAME_MAX + PADDING_LEN)

// usando long long pra (tentar) sobreviver ao ano 2038
long long timestamp()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

int cria_raw_socket(char* nome_interface_rede) {
    // Cria arquivo para o socket sem qualquer protocolo
    int soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); 
    if (soquete == -1) {
        fprintf(stderr, "Erro ao criar socket: Verifique se você é root!\n"); 
        exit(-1); 
    }

    int ifindex = if_nametoindex(nome_interface_rede);
    
    struct sockaddr_ll endereco = {0}; 
    endereco.sll_family = AF_PACKET; 
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ifindex; 

    // Inicializa socket
    if (bind(soquete, (struct sockaddr*) &endereco, sizeof(endereco)) == -1) { 
        fprintf(stderr, "Erro ao fazer bind no socket\n");
        exit(-1); 
    }

    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifindex;
    mr.mr_type = PACKET_MR_PROMISC;

    // Não joga fora o que identifica como lixo: Modo promíscuo
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        fprintf(stderr, "Erro ao fazer setsockopt: "
                        "Verifique se a interface de rede foi especificada corretamente.\n");
        exit(-1);
    }

    return soquete;
}

int envia_mensagem(int soquete, const PacmanPacket *pacote) {
    unsigned char buffer[BUFFER_MAX];

    /* Padding ("cabeçalho Ethernet fake")
    porque as placas de rede reclamaram ao testar sem isso */
    unsigned char padding[PADDING_LEN] = {0};

    memcpy(buffer, padding, PADDING_LEN);

    int tamanho_serializado = serialize_packet(pacote, buffer + PADDING_LEN);

    if (tamanho_serializado < 0) {
        return -1;
    }

    int total = PADDING_LEN + tamanho_serializado;
    if (send(soquete, buffer, total, 0) == -1)
    {
        perror("Erro no send");
        return -1;
    }

    return tamanho_serializado;
}

// retorna -1 se deu timeout, ou quantidade de bytes lidos
int recebe_mensagem(int soquete, int timeoutMillis, PacmanPacket *pacote_recebido) {
    /* Importante: pacote_recebido->dados aponta para um buffer interno que é
    reutilizado a cada chamada. Necessário consumir os dados antes do próximo recebe_mensagem */
    static unsigned char buffer[BUFFER_MAX];
    long long comeco = timestamp();

    struct timeval timeout = {.tv_sec = timeoutMillis / 1000, .tv_usec = (timeoutMillis % 1000) * 1000};
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

    int bytes_lidos;

    do
    {
        bytes_lidos = recv(soquete, buffer, sizeof(buffer), 0);

        if (bytes_lidos > PADDING_LEN)
        {
            // O marcador agora está após o cabeçalho Ethernet
            if (buffer[PADDING_LEN] == START_MARKER)
            {
                int resultado_dsp = deserialize_packet(buffer + PADDING_LEN, bytes_lidos - PADDING_LEN, pacote_recebido);
                if (resultado_dsp == PKT_OK)
                {
                    return bytes_lidos;
                }
                if (resultado_dsp == PKT_CRC_ERRO)
                {
                    PacmanPacket nack = {.tamanho = 0, .sequencia = 0, .tipo = MSG_NACK};
                    envia_mensagem(soquete, &nack);
                }
                // if (resultado_dsp == PKT_INVALIDO) ignora e o receptor que lide com o timeout
            }
        }
    } while (timestamp() - comeco <= timeoutMillis);

    return -1;
}

int envia_com_ack(int soquete, const PacmanPacket *pacote, int timeoutMillis, int max_tentativas) {
    PacmanPacket resposta;
    for (int tentativa = 0; tentativa < max_tentativas; tentativa++) {
        envia_mensagem(soquete, pacote);
        if (recebe_mensagem(soquete, timeoutMillis, &resposta) > 0 && resposta.tipo == MSG_ACK)
            return 0;
        // timeout, NACK ou pacote inesperado: retransmite
    }
    return -1;
}
