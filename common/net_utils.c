#include "net_utils.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // per memset

void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

// Helper per stampare errori SSL specifici
void log_ssl_error(const char* msg) {
    fprintf(stderr, "[SSL ERROR] %s\n", msg);
    ERR_print_errors_fp(stderr);
}

SSL_CTX* create_context(const SSL_METHOD* method) {
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx) {
        log_ssl_error("Unable to create SSL context");
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void configure_context(SSL_CTX *ctx, const char* cert_file, const char* key_file) {
    if (SSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM) <= 0) {
        log_ssl_error("Error loading certificate");
        exit(EXIT_FAILURE);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
        log_ssl_error("Error loading private key");
        exit(EXIT_FAILURE);
    }
}

int send_packet(SSL *ssl, uint8_t type, void *data, uint16_t len) {
    PacketHeader header;
    header.magic = KVM_MAGIC;
    header.type = type;
    header.length = htons(len);

    int written = SSL_write(ssl, &header, sizeof(header));
    if (written <= 0) return -1;

    if (len > 0 && data != NULL) {
        written = SSL_write(ssl, data, len);
        if (written <= 0) return -1;
    }
    return 0;
}
