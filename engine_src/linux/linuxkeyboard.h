#ifndef LINUXKEYBOARD_H
#define LINUXKEYBOARD_H

#include <stdint.h>

#include "userinput.h"

uint32_t linux_keycode_to_tokone_keycode(const uint32_t linux_key);

#endif // LINUXKEYBOARD_H
