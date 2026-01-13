#ifndef NET_UTILS_H
#define NET_UTILS_H
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "protocol.h"

void init_openssl();
SSL_CTX* create_context(const SSL_METHOD* method);
void configure_context(SSL_CTX *ctx, const char* cert_file, const char* key_file);
int send_packet(SSL *ssl, uint8_t type, void *data, uint16_t len);

#endif