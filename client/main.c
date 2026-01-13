#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <sys/time.h> // Necessario per timeval
#include "../common/net_utils.h"
#include "../common/protocol.h"
#include "capturer.h"

int main(int argc, char **argv) {
    // Supportiamo: ./client <IP> <KB> [MOUSE]
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Uso: %s <IP_SERVER> <KB_DEV_PATH> [MOUSE_DEV_PATH]\n", argv[0]);
        fprintf(stderr, "Esempio: sudo %s 192.168.1.50 /dev/input/event3 /dev/input/event4\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    const char *kb_dev = argv[2];
    const char *mouse_dev = (argc == 4) ? argv[3] : NULL;

    printf("[1/5] Inizializzazione OpenSSL...\n");
    init_openssl();
    SSL_CTX *ctx = create_context(SSLv23_client_method());

    printf("[2/5] Creazione Socket TCP...\n");
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[ERROR] Socket creation failed");
        return 1;
    }

    // --- AGGIUNTA: Timeout di 5 secondi ---
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        perror("[WARNING] Cannot set RECV timeout");
    
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
        perror("[WARNING] Cannot set SEND timeout");
    // --------------------------------------

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(KVM_PORT);
    
    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) <= 0) {
        fprintf(stderr, "[ERROR] IP Address non valido: %s\n", argv[1]);
        return 1;
    }

    printf("[3/5] Tentativo di connessione a %s:%d (Timeout 5s)...\n", argv[1], KVM_PORT);
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        perror("[ERROR] Connect fallita (Controlla IP, Firewall o se il Server è attivo)");
        close(sock);
        return 1;
    }
    printf("      >>> Connessione TCP stabilita!\n");

    printf("[4/5] Avvio Handshake SSL/TLS...\n");
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);

    if (SSL_connect(ssl) <= 0) {
        fprintf(stderr, "[ERROR] SSL Handshake fallito.\n");
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(sock);
        return 1;
    }
    printf("      >>> Canale Criptato Attivo (Cipher: %s)\n", SSL_get_cipher(ssl));

    printf("[5/5] Invio Auth e avvio cattura (KB: %s, Mouse: %s)...\n", kb_dev, mouse_dev ? mouse_dev : "Nessuno");
    send_packet(ssl, MSG_AUTH, AUTH_SECRET, strlen(AUTH_SECRET));

    start_capture_loop(kb_dev, mouse_dev, ssl);

    // Pulizia
    SSL_free(ssl);
    close(sock);
    SSL_CTX_free(ctx);
    return 0;
}
