#ifndef LOGGER_H
#define LOGGER_H

#include "../shared/protocol.h"

void log_recebido(const PacmanPacket *pkt);
void log_enviado(const PacmanPacket *pkt);
void log_info(const char *msg);
void log_erro(const char *msg);

#endif