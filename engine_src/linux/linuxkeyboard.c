#include "linuxkeyboard.h"

uint32_t linux_keycode_to_tokone_keycode(const uint32_t linux_key)
{
    #ifndef LOGGER_IGNORE_ASSERTS
    char err_msg[128];
    #endif
    
    switch (linux_key) {
        case ( 24):
            return TOK_KEY_ENTER;
        case ( 25):
            return TOK_KEY_W;
        case ( 26):
            return TOK_KEY_E;
        case ( 28):
            return TOK_KEY_T;
        case ( 31):
            return TOK_KEY_I;
        case ( 36):
            return TOK_KEY_ENTER;
        case ( 38):
            return TOK_KEY_Q;
        case ( 39):
            return TOK_KEY_S;
        case ( 41):
            return TOK_KEY_SPACEBAR;
        case ( 53):
            return TOK_KEY_X;
        case ( 61):
            return TOK_KEY_BACKSLASH;
        case ( 65):
            return TOK_KEY_SPACEBAR;
        case ( 72):
            return TOK_KEY_RIGHTARROW;
        case ( 97):
            return TOK_KEY_UNDERSCORE;
        case (111):
            return TOK_KEY_UPARROW;
        case (113):
            return TOK_KEY_LEFTARROW;
        case (114):
            return TOK_KEY_RIGHTARROW;
        case (116):
            return TOK_KEY_DOWNARROW;
        default:
            #ifndef LOGGER_IGNORE_ASSERTS
            strcpy_capped(err_msg, 128, "unhandled linux keycode: ");
            strcat_uint_capped(err_msg, 128, linux_key);
            strcat_capped(err_msg, 128, "\n");
            #endif
            break;
    }
    
    #ifndef LOGGER_IGNORE_ASSERTS
    log_dump_and_crash(err_msg);
    #endif
    
    return TOK_KEY_ESCAPE;
}

