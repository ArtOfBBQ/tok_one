#include "T1_std.h"
#include "T1_assert.h"

#if T1_ASSERTS_ACTIVE == T1_ACTIVE
#elif T1_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif

/*
Plain loops: the compiler turns them into vector stores (or a call to its
own memset/memcpy) where the CPU has them, no intrinsics needed
*/
void * T1_std_memset(
    void * in,
    s32 value,
    u64 size_bytes)
{
    T1_assert(in != NULL);
    s8 * input = (s8 *)in;
    const s8 byte = (s8)value;
    for (u64 i = 0; i < size_bytes; i++) { input[i] = byte; }
    return in;
}

void *
T1_std_memcpy(
    void * restrict dest,
    const void * restrict src,
    u64 n_bytes)
{
    T1_assert(n_bytes == 0 ||
        (const s8 *)dest + n_bytes <= (const s8 *)src ||
        (const s8 *)src + n_bytes <= (const s8 *)dest);
    s8 * restrict destination = (s8 *)dest;
    const s8 * restrict source = (const s8 *)src;
    for (u64 i = 0; i < n_bytes; i++) { destination[i] = source[i]; }
    return dest;
}

void
T1_std_strcat_cap(
    char * recipient,
    const u32 recipient_size,
    const char * to_append)
{
    u32 i = 0;
    while (recipient[i] != '\0') {
        #if T1_ASSERTS_ACTIVE == T1_ACTIVE
        T1_assert(i < recipient_size);
        #elif T1_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        i++;
    }
    
    u32 j = 0;
    while (to_append[j] != '\0') {
        #if T1_ASSERTS_ACTIVE == T1_ACTIVE
        T1_assert(i < recipient_size - 1);
        #elif T1_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        recipient[i++] = to_append[j++];
    }
    
    recipient[i] = '\0';
}

void
T1_std_strcat_u32_cap(
    char * recipient,
    u32 recipient_size,
    u32 to_append)
{
    u32 i = 0;
    while (recipient[i] != '\0') {
        #if T1_ASSERTS_ACTIVE == T1_ACTIVE
        T1_assert(i < recipient_size);
        #elif T1_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        i++;
    }
    
    T1_std_u32_to_string(to_append, recipient + i);
}

void
T1_std_strcpy_cap(
    char * recipient,
    const u32 recipient_cap,
    const char * origin)
{
    #if T1_ASSERTS_ACTIVE == T1_ACTIVE
    T1_assert(recipient != NULL);
    T1_assert(origin != NULL);
    #elif T1_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    u32 i = 0;
    while (origin[i] != '\0')
    {
        #if T1_ASSERTS_ACTIVE == T1_ACTIVE
        T1_assert(i < recipient_cap);
        #elif T1_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        recipient[i] = origin[i];
        i += 1;
    }
    
    #if T1_ASSERTS_ACTIVE == T1_ACTIVE
    T1_assert(i <= recipient_cap);
    #elif T1_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    recipient[i] = '\0';
}

#if 0
void
T1_std_copy_strings(
    char * recipient,
    const u32 recipient_size,
    const char * origin,
    const u32 origin_size)
{
    u32 i = 0;
    for (; i < origin_size; i++) {
        recipient[i] = origin[i];
    }
    
    for (; i < recipient_size; i++) {
        recipient[i] = '\0';
    }
}
#endif

u64 T1_std_strlen(const char * null_terminated_string)
{
    u32 return_value = 0;
    
    while (
        null_terminated_string[return_value] != '\0')
    {
        return_value++;
    }
    
    return return_value;
}

b8 T1_std_are_equal_strings(
    const char * str1,
    const char * str2)
{
    if (str1 == NULL || str2 == NULL) {
        return 0;
    }
    
    if (
        str1[0] == '\0'
        && str2[0] != '\0')
    {
        return 0;
    }
    
    u32 i = 0;
    while (str1[i] != '\0') {
        if (str1[i] != str2[i]) {
            return 0;
        }
        i++;
    }
    
    if (str1[i] != '\0' || str2[i] != '\0') {
        return 0;
    }
    
    return 1;
}

u8 T1_std_are_equal_strings_of_length(
    const char * str1,
    const char * str2,
    const u64 len)
{
    for (u64 i = 0; i < len; i++) {
        if (str1[i] != str2[i]) {
            return 0;
        }
    }
    
    return 1;
}

void T1_std_u32_to_string(
    const u32 input,
    char * recipient)
{
    if (input == 0) {
        recipient[0] = '0';
        recipient[1] = '\0';
        return;
    }
    
    u32 i = 0;
    u32 start_i = 0;
    u32 end_i;
    
    u64 decimal = 1;
    u32 input_div_dec = input;
    while (input_div_dec > 0) {
        u32 isolated_num = input % (decimal * 10);
        isolated_num /= decimal;
        recipient[i] = (char)('0' + isolated_num);
        i += 1;
        
        decimal *= 10;
        input_div_dec = input / decimal;
    }
    end_i = i - 1;
    
    recipient[end_i + 1] = '\0';
    
    // if we started with an input of -32, we should now have
    // the resut '-' '2' '3', with start_i at '2' and end_i at '3'
    // we want to swap from start_i to end_i
    while (start_i < end_i) {
        char swap = recipient[start_i];
        recipient[start_i] = recipient[end_i];
        recipient[end_i] = swap;
        end_i -= 1;
        start_i += 1; 
    }
}
