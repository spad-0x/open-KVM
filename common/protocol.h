#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define KVM_PORT 8443
#define KVM_MAGIC 0x4B // 'K'
#define AUTH_SECRET "SecretToken123" // Hardcoded per semplicità

typedef enum {
    MSG_AUTH = 0x01,
    MSG_MOUSE_MOVE = 0x10,
    MSG_KEY_EVENT  = 0x11,   // Per tasti speciali (Cmd, Ctrl, F1...)
    MSG_MOUSE_SCROLL = 0x12,
    MSG_TEXT_INPUT = 0x13    // <--- NUOVO: Per testo puro (A, @, €)
} MsgType;

// Header pacchetto (Packed = niente padding del compilatore)
typedef struct __attribute__((packed)) {
    uint8_t magic;
    uint8_t type;
    uint16_t length;
} PacketHeader;

// Payload Mouse
typedef struct __attribute__((packed)) {
    int32_t dx;
    int32_t dy;
} MousePayload;

// Payload Tastiera
typedef struct __attribute__((packed)) {
    uint16_t code;
    uint16_t value; // 0=Up, 1=Down, 2=Repeat
} KeyPayload;

// Payload Scroll
typedef struct __attribute__((packed)) {
    int32_t sx; // Scroll orizzontale
    int32_t sy; // Scroll verticale
} ScrollPayload;

// Text Payload
typedef struct __attribute__((packed)) {
    uint32_t ucs4; // Codice Unicode (supporta anche Emoji se volessi)
} TextPayload;

#endif // PROTOCOL_H