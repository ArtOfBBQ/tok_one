#ifndef T1_STICKY_ERROR_H
#define T1_STICKY_ERROR_H

void T1_sticky_error_new(
    char ** const sticky_error,
    const char * with_string);

void T1_sticky_error_prepend_if_bad(
    char ** const sticky_error,
    const char * to_append);

#endif
