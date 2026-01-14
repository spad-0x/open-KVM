#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "../common/net_utils.h"
#include "../common/protocol.h"
#include "injector.h"

void handle_client(SSL *ssl, const char* client_ip) {
    PacketHeader header;
    uint8_t buffer[1024];

    printf("[%s] In attesa di pacchetti...\n", client_ip);

    while (1) {
        int bytes = SSL_read(ssl, &header, sizeof(header));
        if (bytes <= 0) {
            int err = SSL_get_error(ssl, bytes);
            if (err == SSL_ERROR_ZERO_RETURN) {
                printf("[%s] Disconnessione pulita dal client.\n", client_ip);
            } else {
                printf("[%s] Connessione persa (Error: %d).\n", client_ip, err);
            }
            break;
        }

        uint16_t len = ntohs(header.length);
        if (len > sizeof(buffer)) {
             printf("[%s] ERRORE: Pacchetto troppo grande (%d bytes). Drop.\n", client_ip, len);
             break;
        }

        if (len > 0) {
            if (SSL_read(ssl, buffer, len) <= 0) break;
        }

        if (header.magic != KVM_MAGIC) {
            printf("[%s] ERRORE: Magic byte errato. Ignoro.\n", client_ip);
            continue;
        }

        switch (header.type) {
            case MSG_AUTH:
                printf("[%s] Autenticazione ricevuta.\n", client_ip);
                break;
            case MSG_MOUSE_MOVE: {
                MousePayload *mp = (MousePayload*)buffer;
                
                // Riconvertiamo da Network Order a Host Order
                // Importante: cast a int32_t per riavere il segno (numeri negativi)
                int32_t val_dx = (int32_t)ntohl(mp->dx);
                int32_t val_dy = (int32_t)ntohl(mp->dy);

                // DEBUG: Vediamo cosa arriva
                // printf("[%s] Mouse RAW: dx=%d, dy=%d\n", client_ip, val_dx, val_dy);
                
                // Filtro anti-rumore: a volte arrivano pacchetti 0,0 inutili
                if (val_dx != 0 || val_dy != 0) {
                    inject_mouse_move(val_dx, val_dy);
                }
                break; 
            }
            case MSG_KEY_EVENT: {
                KeyPayload *kp = (KeyPayload*)buffer;
                printf("[%s] Key: Code %d, Val %d\n", client_ip, ntohs(kp->code), ntohs(kp->value));
                inject_key_event(ntohs(kp->code), ntohs(kp->value));
                break;
            }
            case MSG_MOUSE_SCROLL: {
                ScrollPayload *sp = (ScrollPayload*)buffer;
                int32_t val_sx = (int32_t)ntohl(sp->sx);
                int32_t val_sy = (int32_t)ntohl(sp->sy);
                inject_scroll(val_sx, val_sy);
                break;
            }
            case MSG_TEXT_INPUT: {
                TextPayload *tp = (TextPayload*)buffer;
                inject_text(ntohl(tp->ucs4));
                break;
            }
        }
    }
}

int main() {
    init_openssl();
    SSL_CTX *ctx = create_context(SSLv23_server_method());
    configure_context(ctx, "certs/keys/server.crt", "certs/keys/server.key");

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(KVM_PORT), .sin_addr.s_addr = INADDR_ANY };
    
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        perror("Bind failed");
        return 1;
    }
    listen(server_sock, 1);

    printf("=== SERVER AVVIATO (Porta %d) ===\n", KVM_PORT);
    printf("In attesa di connessioni...\n");

    while(1) {
        struct sockaddr_in c_addr;
        socklen_t len = sizeof(c_addr);
        int client = accept(server_sock, (struct sockaddr*)&c_addr, &len);
        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &c_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        
        printf("\n>>> Nuova connessione TCP da: %s\n", client_ip);

        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client);

        if (SSL_accept(ssl) <= 0) {
            printf("[%s] SSL Handshake fallito.\n", client_ip);
            ERR_print_errors_fp(stderr);
        } else {
            printf("[%s] Handshake SSL OK. Avvio loop.\n", client_ip);
            handle_client(ssl, client_ip);
        }
        
        SSL_free(ssl);
        close(client);
        printf("[%s] Sessione terminata. In attesa del prossimo client...\n", client_ip);
    }
}
