#include "T1_assert.h"

#include "T1_os.h"

void T1_crash(const char * crash_message) {
    // stop right here, so the debugger shows where things went wrong
    __builtin_trap();
}
