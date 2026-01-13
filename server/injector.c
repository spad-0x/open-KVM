#include "injector.h"
#include "linux_defs.h"
#include "keymap.h" // Includiamo la nuova mappa
#include <ApplicationServices/ApplicationServices.h>
#include <stdio.h>

void inject_mouse_move(int32_t dx, int32_t dy) {
    // 1. Ottieni la posizione CORRENTE del mouse
    CGEventRef event = CGEventCreate(NULL);
    if (!event) {
        fprintf(stderr, "Err: Impossibile ottenere stato mouse\n");
        return;
    }

    CGPoint cursor = CGEventGetLocation(event);
    CFRelease(event);

    cursor.x += (dx * 1.5);
    cursor.y += (dy * 1.5);

    // 3. Crea ed esegui l'evento movimento
    CGEventRef move = CGEventCreateMouseEvent(
        NULL,
        kCGEventMouseMoved,
        cursor,
        kCGMouseButtonLeft // Il bottone è ignorato per l'evento Moved
    );

    CGEventPost(kCGHIDEventTap, move);
    CFRelease(move);
}

void inject_key_event(uint16_t linux_code, uint16_t value) {
    // --- GESTIONE CLICK MOUSE ---
    if (linux_code >= BTN_LEFT && linux_code <= BTN_MIDDLE) {
        CGMouseButton button;
        if (linux_code == BTN_LEFT) button = kCGMouseButtonLeft;
        else if (linux_code == BTN_RIGHT) button = kCGMouseButtonRight;
        else button = kCGMouseButtonCenter;

        CGEventType type;
        if (value == 1) type = (button == kCGMouseButtonLeft) ? kCGEventLeftMouseDown : kCGEventRightMouseDown;
        else if (value == 0) type = (button == kCGMouseButtonLeft) ? kCGEventLeftMouseUp : kCGEventRightMouseUp;
        else return; // Ignora repeat per i click

        // Ottieni posizione corrente per fare click "qui"
        CGEventRef event = CGEventCreate(NULL);
        CGPoint cursor = CGEventGetLocation(event);
        CFRelease(event);

        CGEventRef click = CGEventCreateMouseEvent(NULL, type, cursor, button);
        CGEventPost(kCGHIDEventTap, click);
        CFRelease(click);
        return; // Abbiamo gestito l'evento, esci.
    }
    
    // Ignora eventi interni del trackpad (es. "dito appoggiato")
    if (linux_code == BTN_TOUCH) return;

    // --- GESTIONE TASTIERA (Codice esistente) ---
    int mac_code = linux_to_mac_keycode(linux_code);
    if (mac_code == -1) return;

    bool down = (value == 1 || value == 2); 
    CGEventRef key = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)mac_code, down);
    CGEventPost(kCGHIDEventTap, key);
    CFRelease(key);
}

void inject_scroll(int32_t sx, int32_t sy) {
    // macOS è sensibile. I valori raw del trackpad sono alti (es. 50 pixel).
    // Lo scroll wheel event si aspetta valori più bassi (es. 1-10) o pixel.
    // Dividiamo per ridurre la velocità, altrimenti scatta troppo.
    // Inoltre su Mac lo scroll "naturale" inverte spesso la Y.
    
    int32_t wheel_y = sy; 
    int32_t wheel_x = sx;

    // Creiamo evento scroll.
    // Parametri: Source, Units (Pixel o Linee), WheelCount, Wheel1 (Y), Wheel2 (X)
    CGEventRef scroll = CGEventCreateScrollWheelEvent(
        NULL,
        kCGScrollEventUnitPixel, // Usiamo Pixel per fluidità
        2, // Numero assi (Y e X)
        wheel_y,
        wheel_x
    );

    CGEventPost(kCGHIDEventTap, scroll);
    CFRelease(scroll);
}