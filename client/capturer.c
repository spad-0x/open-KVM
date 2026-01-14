#include "capturer.h"
#include "../common/protocol.h"
#include "../common/net_utils.h"
#include <linux/input.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <poll.h>
#include <stdlib.h>
#include <ctype.h> // per toupper

#define TOGGLE_KEY 97 
#define JITTER_THRESHOLD 2
#define SCROLL_DIVIDER 3

// Mappa semplificata IT: [KEYCODE] -> {Normal, Shifted, AltGr}
// 0 = Nessun carattere (gestito come tasto raw)
typedef struct {
    uint32_t normal;
    uint32_t shifted;
    uint32_t altgr;
} KeyMap;

static KeyMap layout_it[128];

void init_layout_it() {
    // Inizializza tutto a 0
    for(int i=0; i<128; i++) { layout_it[i].normal=0; layout_it[i].shifted=0; layout_it[i].altgr=0; }

    // Lettere A-Z
    for (int i = KEY_Q; i <= KEY_P; i++) { // Prima riga QWERTY
         // Nota: l'ordine dei keycode è sparso, usiamo una logica manuale per le lettere è meglio
    }
    // Mappatura manuale essenziale Layout IT
    
    // Numeri
    layout_it[KEY_1].normal='1'; layout_it[KEY_1].shifted='!';
    layout_it[KEY_2].normal='2'; layout_it[KEY_2].shifted='"'; layout_it[KEY_2].altgr=0x20AC; // Euro
    layout_it[KEY_3].normal='3'; layout_it[KEY_3].shifted=0xA3; // Sterlina (Latin1)
    layout_it[KEY_4].normal='4'; layout_it[KEY_4].shifted='$';
    layout_it[KEY_5].normal='5'; layout_it[KEY_5].shifted='%';
    layout_it[KEY_6].normal='6'; layout_it[KEY_6].shifted='&';
    layout_it[KEY_7].normal='7'; layout_it[KEY_7].shifted='/';
    layout_it[KEY_8].normal='8'; layout_it[KEY_8].shifted='(';
    layout_it[KEY_9].normal='9'; layout_it[KEY_9].shifted=')';
    layout_it[KEY_0].normal='0'; layout_it[KEY_0].shifted='=';

    // Simboli IT
    layout_it[KEY_MINUS].normal='\''; layout_it[KEY_MINUS].shifted='?'; // Tasto ' ?
    layout_it[KEY_EQUAL].normal=0xEC; layout_it[KEY_EQUAL].shifted='^'; // Tasto ì ^
    layout_it[KEY_LEFTBRACE].normal=0xE8; layout_it[KEY_LEFTBRACE].shifted=0xE9; layout_it[KEY_LEFTBRACE].altgr='['; // è é [
    layout_it[KEY_RIGHTBRACE].normal='+'; layout_it[KEY_RIGHTBRACE].shifted='*'; layout_it[KEY_RIGHTBRACE].altgr=']'; // + * ]
    layout_it[KEY_SEMICOLON].normal=0xF2; layout_it[KEY_SEMICOLON].shifted=0xE7; layout_it[KEY_SEMICOLON].altgr='@'; // ò ç @  <-- QUI LA CHIOCCIOLA
    layout_it[KEY_APOSTROPHE].normal=0xE0; layout_it[KEY_APOSTROPHE].shifted=0xB0; layout_it[KEY_APOSTROPHE].altgr='#'; // à ° #
    layout_it[KEY_BACKSLASH].normal=0xF9; layout_it[KEY_BACKSLASH].shifted=0xA7; // ù §
    layout_it[KEY_COMMA].normal=','; layout_it[KEY_COMMA].shifted=';';
    layout_it[KEY_DOT].normal='.'; layout_it[KEY_DOT].shifted=':';
    layout_it[KEY_SLASH].normal='-'; layout_it[KEY_SLASH].shifted='_';
    layout_it[KEY_SPACE].normal=' '; layout_it[KEY_SPACE].shifted=' ';
    layout_it[KEY_102ND].normal='<'; layout_it[KEY_102ND].shifted='>'; // Tasto <> a sinistra di Z
}

// Funzione helper per le lettere (codici sparsi in linux/input.h)
char get_letter_char(int code) {
    // Mappa evdev -> ASCII per lettere
    const char *keys = "??1234567890-=\b\tqwertyuiop[]\n?asdfghjkl;'`?\\zxcvbnm,./"; 
    // Questa stringa è per layout US base, noi gestiamo solo A-Z qui
    if (code == KEY_A) return 'a'; if (code == KEY_B) return 'b'; if (code == KEY_C) return 'c';
    if (code == KEY_D) return 'd'; if (code == KEY_E) return 'e'; if (code == KEY_F) return 'f';
    if (code == KEY_G) return 'g'; if (code == KEY_H) return 'h'; if (code == KEY_I) return 'i';
    if (code == KEY_J) return 'j'; if (code == KEY_K) return 'k'; if (code == KEY_L) return 'l';
    if (code == KEY_M) return 'm'; if (code == KEY_N) return 'n'; if (code == KEY_O) return 'o';
    if (code == KEY_P) return 'p'; if (code == KEY_Q) return 'q'; if (code == KEY_R) return 'r';
    if (code == KEY_S) return 's'; if (code == KEY_T) return 't'; if (code == KEY_U) return 'u';
    if (code == KEY_V) return 'v'; if (code == KEY_W) return 'w'; if (code == KEY_X) return 'x';
    if (code == KEY_Y) return 'y'; if (code == KEY_Z) return 'z';
    return 0;
}

void set_grab(int fd, int enable) { if (fd != -1) ioctl(fd, EVIOCGRAB, enable); }

void start_capture_loop(const char *kb_path, const char *mouse_path, SSL *ssl) {
    init_layout_it(); // Inizializza mappa

    int fd_kb = open(kb_path, O_RDONLY);
    int fd_mouse = mouse_path ? open(mouse_path, O_RDONLY) : -1;
    if (fd_kb == -1) return;

    struct pollfd fds[2] = { {fd_kb, POLLIN, 0}, {fd_mouse, POLLIN, 0} };
    struct input_event ev;
    bool active_mode = false;
    
    // Stato modificatori locali
    bool shift = false;
    bool altgr = false;
    bool capslock = false; // (Semplificato: resetta se esci)
    bool cmd_down = false; // Tracciamo CMD/CTRL per bypassare il testo

    // Stato trackpad
    int last_abs_x = -1, last_abs_y = -1, finger_count = 0;
    int32_t pending_dx = 0, pending_dy = 0;

    printf("\n=== KVM CLIENT (IT LAYOUT LOCAL PROCESSING) ===\n");

    while (poll(fds, 2, -1) >= 0) {
        for (int i = 0; i < 2; i++) {
            if (fds[i].fd == -1 || (fds[i].revents & POLLIN) == 0) continue;
            if (read(fds[i].fd, &ev, sizeof(ev)) <= 0) continue;

            // 1. GESTIONE TOGGLE
            if (i == 0 && ev.type == EV_KEY && ev.code == TOGGLE_KEY && ev.value == 1) {
                active_mode = !active_mode;
                set_grab(fd_kb, active_mode);
                set_grab(fd_mouse, active_mode);
                // Reset stati critici
                shift = false; altgr = false; cmd_down = false;
                printf(active_mode ? "\r[ REMOTE ]" : "\r[ LOCAL  ]"); fflush(stdout);
                continue;
            }

            if (!active_mode) continue;

            // 2. GESTIONE INPUT
            if (i == 0 && ev.type == EV_KEY) {
                // Aggiorna stato modificatori
                if (ev.code == KEY_LEFTSHIFT || ev.code == KEY_RIGHTSHIFT) shift = (ev.value != 0);
                if (ev.code == KEY_RIGHTALT) altgr = (ev.value != 0); // Right Alt è AltGr
                if (ev.code == KEY_LEFTMETA || ev.code == KEY_RIGHTMETA) cmd_down = (ev.value != 0);
                if (ev.code == KEY_LEFTCTRL || ev.code == KEY_RIGHTCTRL) cmd_down = (ev.value != 0); // Trattiamo anche Ctrl come command system
                if (ev.code == KEY_CAPSLOCK && ev.value == 1) capslock = !capslock;

                // --- LOGICA IBRIDA ---
                uint32_t char_to_send = 0;
                
                // Se premo CMD/CTRL, VOGLIO il comportamento RAW (es. Cmd+C, Cmd+Tab)
                // Se è un rilascio (value=0), inviamo sempre RAW per evitare tasti incastrati
                if (!cmd_down && ev.value == 1) {
                    
                    // Controlla lettere A-Z
                    char letter = get_letter_char(ev.code);
                    if (letter) {
                        bool is_upper = (shift ^ capslock);
                        char_to_send = is_upper ? toupper(letter) : letter;
                    } 
                    // Controlla altri tasti mappati (Numeri, simboli IT)
                    else if (ev.code < 128) {
                        if (altgr && layout_it[ev.code].altgr) char_to_send = layout_it[ev.code].altgr;
                        else if (shift && layout_it[ev.code].shifted) char_to_send = layout_it[ev.code].shifted;
                        else if (!shift && !altgr && layout_it[ev.code].normal) char_to_send = layout_it[ev.code].normal;
                    }
                }

                if (char_to_send != 0) {
                    // INVIO TESTO PURO
                    TextPayload tp;
                    tp.ucs4 = htonl(char_to_send);
                    send_packet(ssl, MSG_TEXT_INPUT, &tp, sizeof(tp));
                } else {
                    // INVIO RAW (Per modificatori, F1-F12, Cmd+Tab, frecce...)
                    KeyPayload kp;
                    kp.code = htons(ev.code);
                    kp.value = htons(ev.value);
                    send_packet(ssl, MSG_KEY_EVENT, &kp, sizeof(kp));
                }
            }
            // ... (Copia qui la logica Mouse/Trackpad identica a prima) ...
             else if (ev.type == EV_ABS) { /* ... codice trackpad ... */ }
             else if (ev.type == EV_REL) { /* ... codice mouse ... */ }
             else if (ev.type == EV_SYN) { /* ... codice sync ... */ }
        }
    }
    set_grab(fd_kb, 0); set_grab(fd_mouse, 0);
    close(fd_kb); if (fd_mouse != -1) close(fd_mouse);
}