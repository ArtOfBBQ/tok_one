#include "linuxkeyboard.h"

uint32_t linux_keycode_to_tokone_keycode(const uint32_t linux_key)
{
    #ifndef LOGGER_IGNORE_ASSERTS
    char err_msg[128];
    #endif
    
    switch (linux_key) {
        case (  9):
            return TOK_KEY_ESCAPE;
        case ( 10):
            return TOK_KEY_1;
        case ( 11):
            return TOK_KEY_2;
        case ( 12):
            return TOK_KEY_3;
        case ( 13):
            return TOK_KEY_4;
        case ( 14):
            return TOK_KEY_5;
        case ( 15):
            return TOK_KEY_6;
        case ( 16):
            return TOK_KEY_7;
        case ( 17):
            return TOK_KEY_8;
        case ( 18):
            return TOK_KEY_9;
        case ( 19):
            return TOK_KEY_0;
        case ( 24):
            return TOK_KEY_ENTER;
        case ( 25):
            return TOK_KEY_W;
        case ( 26):
            return TOK_KEY_E;
        case ( 27):
            return TOK_KEY_R;
        case ( 28):
            return TOK_KEY_T;
        case ( 29):
            return TOK_KEY_Y;
        case ( 30):
            return TOK_KEY_U;
        case ( 31):
            return TOK_KEY_I;
        case ( 32):
            return TOK_KEY_O;
        case ( 33):
            return TOK_KEY_P;
        case ( 36):
            return TOK_KEY_ENTER;
        case ( 38):
            return TOK_KEY_Q;
        case ( 39):
            return TOK_KEY_S;
        case ( 40):
            return TOK_KEY_D;
        case ( 41):
            return TOK_KEY_SPACEBAR;
        case ( 42):
            return TOK_KEY_G;
        case ( 43):
            return TOK_KEY_H;
        case ( 44):
            return TOK_KEY_J;
        case ( 45):
            return TOK_KEY_K;
        case ( 46):
            return TOK_KEY_L;
        case ( 47):
            return TOK_KEY_SEMICOLON;
        case ( 52):
            return TOK_KEY_Z;
        case ( 53):
            return TOK_KEY_X;
        case ( 54):
            return TOK_KEY_C;
        case ( 55):
            return TOK_KEY_V;
        case ( 56):
            return TOK_KEY_B;
        case ( 57):
            return TOK_KEY_N;
        case ( 58):
            return TOK_KEY_M;
        case ( 59):
            return TOK_KEY_COMMA;
        case ( 60):
            return TOK_KEY_FULLSTOP;
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
            printf("unhandled linux keyboard code: %u\n", linux_key);
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

