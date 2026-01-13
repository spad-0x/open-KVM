#include "keymap.h"
#include "linux_defs.h"
#include <stdio.h> // per NULL/definire size se serve

// Lookup table: Index = Linux Keycode, Value = Mac Virtual Keycode
// Usiamo 256 come dimensione sicura per i tasti standard.
static int key_lookup_table[256];
static int initialized = 0;

void init_keymap() {
    if (initialized) return;

    // Inizializziamo tutto a -1 (tasto sconosciuto)
    for (int i = 0; i < 256; i++) key_lookup_table[i] = -1;

    // --- Mappatura ---
    // Sintassi: table[CODICE_LINUX] = CODICE_MAC_HEX;
    
    // Lettere
    key_lookup_table[KEY_A] = 0x00;
    key_lookup_table[KEY_S] = 0x01;
    key_lookup_table[KEY_D] = 0x02;
    key_lookup_table[KEY_F] = 0x03;
    key_lookup_table[KEY_H] = 0x04;
    key_lookup_table[KEY_G] = 0x05;
    key_lookup_table[KEY_Z] = 0x06;
    key_lookup_table[KEY_X] = 0x07;
    key_lookup_table[KEY_C] = 0x08;
    key_lookup_table[KEY_V] = 0x09;
    key_lookup_table[KEY_B] = 0x0B; // Nota: B e V sono invertiti nei codici interni ma non sulla tastiera fisica
    key_lookup_table[KEY_Q] = 0x0C;
    key_lookup_table[KEY_W] = 0x0D;
    key_lookup_table[KEY_E] = 0x0E;
    key_lookup_table[KEY_R] = 0x0F;
    key_lookup_table[KEY_Y] = 0x10;
    key_lookup_table[KEY_T] = 0x11;
    key_lookup_table[KEY_1] = 0x12;
    key_lookup_table[KEY_2] = 0x13;
    key_lookup_table[KEY_3] = 0x14;
    key_lookup_table[KEY_4] = 0x15;
    key_lookup_table[KEY_6] = 0x16;
    key_lookup_table[KEY_5] = 0x17;
    key_lookup_table[KEY_EQUAL] = 0x18;
    key_lookup_table[KEY_9] = 0x19;
    key_lookup_table[KEY_7] = 0x1A;
    key_lookup_table[KEY_MINUS] = 0x1B;
    key_lookup_table[KEY_8] = 0x1C;
    key_lookup_table[KEY_0] = 0x1D;
    key_lookup_table[KEY_RIGHTBRACE] = 0x1E;
    key_lookup_table[KEY_O] = 0x1F;
    key_lookup_table[KEY_U] = 0x20;
    key_lookup_table[KEY_LEFTBRACE] = 0x21;
    key_lookup_table[KEY_I] = 0x22;
    key_lookup_table[KEY_P] = 0x23;
    key_lookup_table[KEY_ENTER] = 0x24; // Return
    key_lookup_table[KEY_L] = 0x25;
    key_lookup_table[KEY_J] = 0x26;
    key_lookup_table[KEY_APOSTROPHE] = 0x27;
    key_lookup_table[KEY_K] = 0x28;
    key_lookup_table[KEY_SEMICOLON] = 0x29;
    key_lookup_table[KEY_BACKSLASH] = 0x2A;
    key_lookup_table[KEY_COMMA] = 0x2B;
    key_lookup_table[KEY_SLASH] = 0x2C;
    key_lookup_table[KEY_N] = 0x2D;
    key_lookup_table[KEY_M] = 0x2E;
    key_lookup_table[KEY_DOT] = 0x2F;
    key_lookup_table[KEY_TAB] = 0x30;
    key_lookup_table[KEY_SPACE] = 0x31;
    key_lookup_table[KEY_GRAVE] = 0x32;
    key_lookup_table[KEY_BACKSPACE] = 0x33;
    key_lookup_table[KEY_ESC] = 0x35;
    
    // Modificatori (IMPORTANTISSIMI)
    key_lookup_table[KEY_LEFTMETA] = 0x37;  // Command (Left)
    key_lookup_table[KEY_RIGHTMETA] = 0x36; // Command (Right) - Spesso mappato a 0x36 o 0x37
    key_lookup_table[KEY_LEFTSHIFT] = 0x38;
    key_lookup_table[KEY_CAPSLOCK] = 0x39;
    key_lookup_table[KEY_LEFTALT] = 0x3A;   // Option
    key_lookup_table[KEY_LEFTCTRL] = 0x3B;  // Control
    key_lookup_table[KEY_RIGHTSHIFT] = 0x3C;
    key_lookup_table[KEY_RIGHTALT] = 0x3D;  // Option Right
    key_lookup_table[KEY_RIGHTCTRL] = 0x3E; // Control Right

    // Frecce
    key_lookup_table[KEY_LEFT] = 0x7B;
    key_lookup_table[KEY_RIGHT] = 0x7C;
    key_lookup_table[KEY_DOWN] = 0x7D;
    key_lookup_table[KEY_UP] = 0x7E;

    // Funzione
    key_lookup_table[KEY_F1] = 0x7A;
    key_lookup_table[KEY_F2] = 0x78;
    key_lookup_table[KEY_F3] = 0x63;
    key_lookup_table[KEY_F4] = 0x76;
    key_lookup_table[KEY_F5] = 0x60;
    key_lookup_table[KEY_F6] = 0x61;
    key_lookup_table[KEY_F7] = 0x62;
    key_lookup_table[KEY_F8] = 0x64;
    key_lookup_table[KEY_F9] = 0x65;
    key_lookup_table[KEY_F10] = 0x6D;
    key_lookup_table[KEY_F11] = 0x67;
    key_lookup_table[KEY_F12] = 0x6F;

    initialized = 1;
}

int linux_to_mac_keycode(uint16_t linux_code) {
    if (!initialized) init_keymap();
    
    if (linux_code >= 256) return -1; // Out of bounds
    return key_lookup_table[linux_code];
}
