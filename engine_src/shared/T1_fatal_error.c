#include "T1_fatal_error.h"

#include "T1_std.h"
#include "T1_mem.h"

void T1_fatal_error_new(
    char ** const fatal_error,
    const char * with_string)
{
    *fatal_error = T1_mem_malloc_unmanaged(512);
    T1_std_memset(*fatal_error, 0, 512);
    T1_std_strcpy_cap(*fatal_error, 512, with_string);
}
