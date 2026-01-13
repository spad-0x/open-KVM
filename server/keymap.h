#ifndef KEYMAP_H
#define KEYMAP_H
#include <stdint.h>

void init_keymap();
int linux_to_mac_keycode(uint16_t linux_code);

#endif
