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

#define TOGGLE_KEY 97       // Right Ctrl Code Event
#define JITTER_THRESHOLD 2  // Jitter Threshold for mouse movement
#define SCROLL_DIVIDER 1    // Scroll Divider

void set_grab(int fd, int enable) {
    if (fd != -1) ioctl(fd, EVIOCGRAB, enable);
}

void start_capture_loop(const char *kb_path, const char *mouse_path, SSL *ssl) {
    int fd_kb = open(kb_path, O_RDONLY);
    if (fd_kb == -1) { perror("Errore Keyboard"); return; }

    int fd_mouse = -1;
    if (mouse_path) {
        fd_mouse = open(mouse_path, O_RDONLY);
        if (fd_mouse == -1) perror("Warning: Mouse non aperto");
    }

    struct pollfd fds[2];
    fds[0].fd = fd_kb;    fds[0].events = POLLIN;
    fds[1].fd = fd_mouse; fds[1].events = POLLIN;

    struct input_event ev;
    bool active_mode = false;

    // Variabili di stato Trackpad
    int last_abs_x = -1, last_abs_y = -1;
    int32_t pending_dx = 0, pending_dy = 0;
    
    // STATO GESTURE
    int finger_count = 0; // 0=Nessuno, 1=Muovi, 2=Scroll, 3=Swipe...

    printf("\n=== KVM CLIENT ===\n");
    printf("Premi RIGHT CTRL per switchare.\n");

    while (poll(fds, 2, -1) >= 0) {
        for (int i = 0; i < 2; i++) {
            if (fds[i].fd == -1 || (fds[i].revents & POLLIN) == 0) continue;
            if (read(fds[i].fd, &ev, sizeof(ev)) <= 0) continue;

            // --- TOGGLE (Tastiera) ---
            if (i == 0 && ev.type == EV_KEY && ev.code == TOGGLE_KEY && ev.value == 1) {
                active_mode = !active_mode;
                set_grab(fd_kb, active_mode);
                set_grab(fd_mouse, active_mode);
                
                // Reset stati
                last_abs_x = -1; last_abs_y = -1;
                pending_dx = 0; pending_dy = 0;
                finger_count = 0;

                if (active_mode) printf("\rSTATO: [ REMOTE >> MAC ]   ");
                else             printf("\rSTATO: [ LOCALE << LINUX ] ");
                fflush(stdout);
                continue;
            }

            if (!active_mode) continue;

            // --- GESTIONE FINGER COUNT ---
            if (ev.type == EV_KEY) {
                // Il kernel invia value=1 quando lo strumento viene rilevato, 0 quando sparisce
                if (ev.code == BTN_TOOL_FINGER && ev.value == 1) finger_count = 1;
                else if (ev.code == BTN_TOOL_DOUBLETAP && ev.value == 1) finger_count = 2;
                else if (ev.code == BTN_TOOL_TRIPLETAP && ev.value == 1) finger_count = 3;
                
                // BTN_TOUCH=0 significa che TUTTE le dita sono state alzate
                if (ev.code == BTN_TOUCH && ev.value == 0) {
                    finger_count = 0;
                    last_abs_x = -1; last_abs_y = -1;
                    pending_dx = 0; pending_dy = 0;
                }
                
                // Invia click fisici solo se siamo con 1 dito (o gestisci right click con 2 dita se vuoi)
                // Qui lasciamo passare i click fisici standard
                if (ev.code >= BTN_MOUSE && ev.code < BTN_JOYSTICK) { // Range click mouse
                     KeyPayload kp;
                     kp.code = htons(ev.code);
                     kp.value = htons(ev.value);
                     send_packet(ssl, MSG_KEY_EVENT, &kp, sizeof(kp));
                }
                // Gestione tastiera
                else if (i == 0 && ev.code != TOGGLE_KEY) {
                    KeyPayload kp;
                    kp.code = htons(ev.code);
                    kp.value = htons(ev.value);
                    send_packet(ssl, MSG_KEY_EVENT, &kp, sizeof(kp));
                }
            }

            // --- GESTIONE MOVIMENTO ACCUMULATO ---
            else if (ev.type == EV_ABS) {
                if (ev.code == ABS_X || ev.code == ABS_MT_POSITION_X) {
                    if (last_abs_x != -1) pending_dx += (ev.value - last_abs_x);
                    last_abs_x = ev.value;
                }
                else if (ev.code == ABS_Y || ev.code == ABS_MT_POSITION_Y) {
                    if (last_abs_y != -1) pending_dy += (ev.value - last_abs_y);
                    last_abs_y = ev.value;
                }
            }
            else if (ev.type == EV_REL) {
                 if (ev.code == REL_X) pending_dx += ev.value;
                 else if (ev.code == REL_Y) pending_dy += ev.value;
                 // Rotella mouse fisica (se colleghi un mouse USB)
                 else if (ev.code == REL_WHEEL) {
                     ScrollPayload sp = {0, 0};
                     sp.sy = htonl(ev.value * 10); // Moltiplichiamo per effetto visibile
                     send_packet(ssl, MSG_MOUSE_SCROLL, &sp, sizeof(sp));
                 }
            }

            // --- SYNC REPORT: DECIDIAMO COSA INVIARE ---
            else if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
                if (pending_dx != 0 || pending_dy != 0) {
                    
                    if (finger_count == 2) {
                        // --- GESTURE SCROLL (2 DITA) ---
                        ScrollPayload sp;
                        // Riduciamo la sensibilità dividendo il delta
                        sp.sx = htonl(pending_dx / SCROLL_DIVIDER);
                        sp.sy = htonl(pending_dy / SCROLL_DIVIDER); 
                        
                        // Inviamo solo se il divisore non ha azzerato tutto
                        if (sp.sx != 0 || sp.sy != 0)
                            send_packet(ssl, MSG_MOUSE_SCROLL, &sp, sizeof(sp));
                    
                    } else if (finger_count == 1 || finger_count == 0) {
                        // --- MOVIMENTO CURSORE (1 DITO) ---
                        // (finger_count 0 serve perché a volte BTN_TOOL arriva dopo il primo movimento)
                        MousePayload mp;
                        mp.dx = htonl(pending_dx);
                        mp.dy = htonl(pending_dy);
                        send_packet(ssl, MSG_MOUSE_MOVE, &mp, sizeof(mp));
                    }
                    
                    // Reset accumulatori frame
                    pending_dx = 0;
                    pending_dy = 0;
                }
            }
        }
    }
    set_grab(fd_kb, 0); set_grab(fd_mouse, 0);
    close(fd_kb); if (fd_mouse != -1) close(fd_mouse);
}