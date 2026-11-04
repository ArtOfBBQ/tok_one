#include "T1_assert.h"

#include "T1_os.h"

void T1_crash(const char * crash_message) {
    T1_os_write_error(crash_message);
    T1_os_write_error("\n");
    // stop right here, so the debugger shows where things went wrong
    __builtin_trap();
}

#if T1_ASSERTS_ACTIVE == T1_ACTIVE
void T1_assert_failed(
    const char * file,
    const uint32_t line,
    const char * condition)
{
    // the line's digits, last first
    char digits[11];
    u32 digits_size = 0;
    u32 rest = line;
    do {
        digits[digits_size++] = (char)('0' + rest % 10);
        rest /= 10;
    } while (rest > 0);
    char line_text[11];
    for (u32 i = 0; i < digits_size; i++) {
        line_text[i] = digits[digits_size - 1 - i];
    }
    line_text[digits_size] = '\0';
    
    T1_os_write_error(file);
    T1_os_write_error(":");
    T1_os_write_error(line_text);
    T1_os_write_error(": T1_assert(");
    T1_os_write_error(condition);
    T1_os_write_error(") failed\n");
    __builtin_trap();
}
#elif T1_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif
