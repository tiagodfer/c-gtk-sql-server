#ifndef SERVER_H
#define SERVER_H

#include <openssl/ssl.h>

void send_chunk(SSL *ssl, const char *data);
int start_server(int port, const char *cpf_path, const char *cnpj_path, const char *interface);
int stop_server();

#endif
