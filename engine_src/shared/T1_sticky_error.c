#include "T1_sticky_error.h"

#include "T1_std.h"
#include "T1_mem.h"

void T1_sticky_error_new(
    char ** const sticky_error,
    const char * with_string)
{
    *sticky_error = T1_mem_malloc_unmanaged(512);
    T1_std_memset(*sticky_error, 0, 512);
    T1_std_strcpy_cap(*sticky_error, 512, with_string);
}

void T1_sticky_error_prepend_if_bad(
    char ** const existing_error,
    const char * to_append)
{
    if (*existing_error == 0) {
        return;
    }
    
    char * new_error = T1_mem_malloc_unmanaged(512);
    T1_std_memset(new_error, 0, 512);
    T1_std_strcpy_cap(new_error, 512, to_append);
    T1_std_strcat_cap(new_error, 512, *existing_error);
    
    *existing_error = new_error;
}
