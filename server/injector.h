#ifndef INJECTOR_H
#define INJECTOR_H
#include <stdint.h>

void inject_mouse_move(int32_t dx, int32_t dy);
void inject_key_event(uint16_t linux_code, uint16_t value);
void inject_scroll(int32_t sx, int32_t sy);
void inject_text(uint32_t ucs4);

#endif