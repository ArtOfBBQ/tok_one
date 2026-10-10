#ifndef T1_FATAL_ERROR_H
#define T1_FATAL_ERROR_H

void T1_fatal_error_new(
    char ** const fatal_error,
    const char * with_string);

void T1_fatal_error_prepend_if_bad(
    char ** const fatal_error,
    const char * to_append);

#endif
