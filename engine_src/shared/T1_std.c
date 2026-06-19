#include "T1_std.h"

#if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
#include <assert.h>
#elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif

#include "T1_simd.h"

#include <string.h>

void * T1_std_memset(
    void * in,
    s32 value,
    u64 size_bytes)
{
    assert(in != NULL);
    
    s8 * input = (s8 *)in;
    
    u32 i = 0;
    
    #ifdef __ARM_NEON
    int8x16_t neon_preset = vld1q_dup_s8(&value);
    for (; i+15 < size_bytes; i += 16) {
	    vst1q_s8(input + i, neon_preset);
    }
    #endif
    
    #ifdef __AVX__
    __m256i avx_preset = _mm256_set1_epi8((char)value);
    
    for (; i+31 < size_bytes; i += 32) {
        _mm256_storeu_si256(
            (__m256i *)(input + i),
            avx_preset);
    }
    #endif
    
    #ifdef __SSE2__
    __m128i sse_preset = _mm_set1_epi8((char)value);
    
    for (; i+15 < size_bytes; i += 16) {
        _mm_storeu_si128(
            (__m128i *)(input + i),
            sse_preset);
    }
    #endif
    
    for (; i < size_bytes; i++) {
        input[i] = (char)value;
    }
    
    return in;
}

void
T1_std_memset_i16(
    void * in,
    s16 value,
    u32 size_bytes)
{
    s16 * input = (s16 *)in;
    
    u32 i = 0;
    
    #ifdef __ARM_NEON
    int16x8_t neon_preset = vld1q_dup_s16(&value);
    for (; i+7 < (size_bytes / 2); i += 8) {
        vst1q_s16(input + i, neon_preset);
    }
    #endif
    
    #ifdef __AVX__
    __m256i avx_preset = _mm256_set1_epi16(value);
    
    for (; i+15 < (size_bytes / 2); i += 16) {
        _mm256_storeu_si256(
            (__m256i *)(input + i),
            avx_preset);
    }
    #endif
    
    #ifdef __SSE2__
    __m128i sse_preset = _mm_set1_epi16(value);
    
    for (; i+7 < (size_bytes / 2); i += 8) {
        _mm_storeu_si128(
            (__m128i *)(input + i),
            sse_preset);
    }
    #endif
    
    for (; i < (size_bytes / 2); i++) {
        input[i] = value;
    }
}

void
T1_std_memset_s32(
    void * in,
    s32 value,
    u32 size_bytes)
{
    s32 * input = (s32 *)in;
    
    u32 i = 0;
    
    #ifdef __ARM_NEON
    int16x8_t neon_preset = vld1q_dup_s32(&value);
    for (; i+3 < (size_bytes / 4); i += 4) {
        vst1q_s32(input + i, neon_preset);
    }
    #endif
    
    #ifdef __AVX__
    __m256i avx_preset = _mm256_set1_epi32(value);
    
    for (; i+7 < (size_bytes / 4); i += 8) {
        _mm256_storeu_si256(
            (__m256i *)(input + i),
            avx_preset);
    }
    #endif
    
    #ifdef __SSE2__
    __m128i sse_preset = _mm_set1_epi32(value);
    
    for (; i+3 < (size_bytes / 4); i += 4) {
        _mm_storeu_si128(
            (__m128i *)(input + i),
            sse_preset);
    }
    #endif
    
    for (; i < (size_bytes / 4); i++) {
        input[i] = value;
    }
}

void
T1_std_memset_f32(
    void * in,
    f32 value,
    u32 size_bytes)
{
    f32 * input = (f32 *)in;
    
    u32 i = 0;
    
    #ifdef __ARM_NEON
    float32x4_t neon_preset = vld1q_dup_f32(&value);
    for (; i+3 < (size_bytes / 4); i += 4) {
        vst1q_f32(input + i, neon_preset);
    }
    #endif
    
    #ifdef __AVX__
    __m256 avx_preset = _mm256_set1_ps(value);
    
    for (; i+7 < (size_bytes / 4); i += 8) {
        _mm256_storeu_ps(
            (input + i),
            avx_preset);
    }
    #endif
    
    #ifdef __SSE2__
    __m128 sse_preset = _mm_set1_ps(value);
    
    for (; i+3 < (size_bytes / 4); i += 4) {
        _mm_storeu_ps(
            (input + i),
            sse_preset);
    }
    #endif
    
    for (; i < (size_bytes / 4); i++) {
        input[i] = value;
    }
}

void *
T1_std_memcpy(
    void * dest,
    const void * src,
    u64 n_bytes)
{
    u32 i = 0;
    char * destination = (char *)dest;
    char * source = (char *)src;
    
    #ifdef __ARM_NEON
    for (; i+15 < n_bytes; i += 16) {
        int8x16_t neon_copy = vld1q_s8(src + i);
        vst1q_s8(dest + i, neon_copy);
    }
    #endif
    
    #ifdef __AVX__
    for (; i+31 < n_bytes; i += 32) {
        __m256 sse_copy = _mm256_loadu_si256((const __m256i *)(source + i));
        _mm256_storeu_si256(
            (__m256i *)(destination + i),
            sse_copy);
    }
    #endif
    
    #ifdef __SSE2__
    for (; i+15 < n_bytes; i += 16) {
        __m128 sse_copy = _mm_loadu_si128((const __m128i *)(source + i));
        _mm_storeu_si128(
            (__m128i *)(destination + i),
            sse_copy);
    }
    #endif
    
    for (; i < n_bytes; i++) {
        destination[i] = source[i];
    }
    
    return dest;
}

f32
T1_std_minf(const f32 x, const f32 y)
{
    return ((x <= y) * x) + ((y < x) * y);
}

f32
T1_std_maxf(const f32 x, const f32 y)
{
    return ((x >  y) * x) + ((x <= y) * y);
}

s32
T1_std_mini(const s32 x, const s32 y)
{
    return
        ((x <= y) * x) +
        ((y < x) * y);
}

int
T1_std_maxi(const s32 x, const s32 y)
{
    return ((x >  y) * x) + ((x <= y) * y);
}

void
T1_std_internal_strcat_cap(
    char * recipient,
    const u32 recipient_size,
    const char * to_append)
{
    u32 i = 0;
    while (recipient[i] != '\0') {
        #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
        assert(i < recipient_size);
        #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        i++;
    }
    
    u32 j = 0;
    while (to_append[j] != '\0') {
        #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
        assert(i < recipient_size - 1);
        #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        recipient[i++] = to_append[j++];
    }
    
    recipient[i] = '\0';
}

void
T1_std_strcat_char_cap(
    char * recipient,
    char to_append)
{
    u32 i = 0;
    while (recipient[i] != '\0') {
        i++;
    }
    
    recipient[i++] = to_append;
    
    recipient[i] = '\0';
}

void
T1_std_internal_strcat_int_cap(
    char * recipient,
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    const u32 recipient_size,
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    const s32 to_append)
{
    u32 i = 0;
    while (recipient[i] != '\0') {
        #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
        assert(i < recipient_size);
        #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        i++;
    }
    
    T1_std_s32_to_string(to_append, recipient + i);
}

void
T1_std_internal_strcat_uint_cap(
    char * recipient,
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    const u32 recipient_size,
    #elif T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    #else
    #error
    #endif
    const u32 to_append)
{
    u32 i = 0;
    while (recipient[i] != '\0') {
        #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
        assert(i < recipient_size);
        #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        i++;
    }
    
    T1_std_u32_to_string(to_append, recipient + i);
}

void
T1_std_internal_strcat_f32_cap(
    char * recipient,
    const u32 recipient_size,
    const f32 to_append)
{
    u32 precision = 4;
    
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    assert(recipient != NULL);
    assert(recipient_size > 0);
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    u64 rlen = T1_std_strlen(recipient);
    
    char * adj_recip = recipient + rlen;
    f32 input = to_append;
    rlen = 0;
    
    if (input < 0.0f) {
        adj_recip[rlen++] = '-';
        adj_recip[rlen] = '\0';
        input *= -1.0f;
    }
    
    u32 precision_mult = 1;
    for (u8 _ = 0; _ < precision; _++) {
        precision_mult *= 10;
    }
    
    f32 temp_above_decimal = (f32)(s32)input;
    u32 above_decimal = (u32)temp_above_decimal;
    // we're adding an extra '1' in front of the fractional part here, to
    // make it easier to work with leading zeros
    u32 below_decimal =
        (u32)(((input - temp_above_decimal)+1.0f) * precision_mult);
    
    T1_std_u32_to_string(
        above_decimal,
        adj_recip + rlen);
    rlen = T1_std_strlen(adj_recip);
    
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    assert(rlen < recipient_size);
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    if (below_decimal > 0) {
        adj_recip[rlen++] = '.';
        
        T1_std_u32_to_string(
            below_decimal,
            adj_recip + rlen);
        
        // We added a '1' at the start before, so we need to remove that
        while (adj_recip[rlen] != '\0') {
            adj_recip[rlen] = adj_recip[rlen + 1];
            rlen += 1;
        }
    }
    
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE == T1_ACTIVE
    assert(adj_recip[0] != '.');
    #elif T1_STD_ASSERTS_ACTIVE == T1_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
}

u64
T1_std_strlcat(
    char * dst,
    const char * source,
    u64 size)
{
    return strlcat(dst, source, size);
}

void
T1_std_internal_strcpy_cap(
    char * recipient,
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    const u32 recipient_cap,
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    const char * origin)
{
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    assert(recipient != NULL);
    assert(origin != NULL);
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    u32 i = 0;
    while (origin[i] != '\0')
    {
        #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
        assert(i < recipient_cap);
        #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        recipient[i] = origin[i];
        i += 1;
    }
    
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    assert(i <= recipient_cap);
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    recipient[i] = '\0';
}

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

__attribute__((no_sanitize("address")))
u64
T1_std_strlen(const char * null_terminated_string)
{
    u32 return_value = 0;
    
    #ifdef __ARM_NEON
    s8 * str_as_i8 = (s8 *)null_terminated_string;
    s8 nullchar = (s8)'\0';
    int8x16_t nullterm = vld1q_dup_s8(&nullchar);
    
    while (1) {
        int8x16_t chunk = vld1q_s8(str_as_i8);
        uint8x16_t matches = vceqq_s8(nullterm, chunk);
        u8 any_match = vmaxvq_u8(matches);
        if (any_match) { break; }
        
        return_value += 16;
        str_as_i8 += 16;
    }
    #endif
    
    while (
        null_terminated_string[return_value] != '\0')
    {
        return_value++;
    }
    
    return return_value;
}

void
T1_std_strtolower(char * in)
{
    s8 offset = ('A' - 'a');
    while (in[0] != '\0') {
        if (in[0] >= 'A' && in[0] <= 'Z') {
            in[0] = in[0] - offset;
        }
        in++;
    }
}

u8
T1_std_string_starts_with(
    const char * str_to_check,
    const char * start)
{
    if (str_to_check == NULL || start == NULL) {
        return false;
    }
    
    s32 i = 0;
    
    while (start[i] != '\0') {
        if (str_to_check[i] != start[i]) {
            return false;
        }
        i++;
    }
    
    return true;
}

u8
T1_std_string_ends_with(
    const char * str_to_check,
    const char * ending)
{
    if (str_to_check == NULL || ending == NULL) {
        return false;
    }
    
    u32 str_to_check_len = (u32)T1_std_strlen(str_to_check);
    u32 ending_len = (u32)T1_std_strlen(ending);
    
    if (ending_len > str_to_check_len || ending_len < 1) {
        return false;
    }
    
    u32 i = str_to_check_len;
    u32 j = ending_len;
    
    while (j > 0) {
        j--;
        i--;
        
        if (str_to_check[i] != ending[j]) {
            return false;
        }
    }
    
    return true;
}

void
T1_std_strsub(
    char * in,
    const char * to_match,
    const char * replacement)
{
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    assert(T1_std_strlen(to_match) >= T1_std_strlen(replacement));
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    u64 replacement_len = T1_std_strlen(replacement);
    
    u32 start_i = 0;
    while (in[start_i] != '\0') {
        
        u32 match = 1;
        u32 match_len = 0;
        u32 j = 0;
        while (to_match[j] != '\0') {
            if (in[start_i + j] != to_match[j]) {
                match = 0;
                break;
            }
            
            match_len += 1;
            j++;
        }
        
        if (match) {
            j = start_i;
            for (; j < (start_i + replacement_len); j++) {
                in[j] = replacement[j-start_i];
            }
            
            j -= 1;
            u32 k = start_i + match_len;
            while (in[k] != '\0') {
                in[j++] = in[k++];
            }
            in[j+1] = '\0';
        }
        
        start_i += 1;
    }
}

u8
T1_std_are_equal_strings(
    const char * str1,
    const char * str2)
{
    if (str1 == NULL || str2 == NULL) {
        return false;
    }
    
    if (
        str1[0] == '\0'
        && str2[0] != '\0')
    {
        return false;
    }
    
    u32 i = 0;
    while (str1[i] != '\0') {
        if (str1[i] != str2[i]) {
            return false;
        }
        i++;
    }
    
    if (str1[i] != '\0' || str2[i] != '\0') {
        return false;
    }

    return true;
}

u8
T1_std_are_equal_until_nullterminator(
    const char * str1,
    const char * str2)
{
    if (str1 == NULL || str2 == NULL) {
        return false;
    }
    
    u32 i = 0;
    while (
        str1[i] != '\0' &&
        str2[i] != '\0')
    {
        if (str1[i] != str2[i]) {
            return false;
        }
        i++;
    }
    
    return true;
}

u8
T1_std_are_equal_strings_of_length(
    const char * str1,
    const char * str2,
    const u64 len)
{
    for (u64 i = 0; i < len; i++) {
        if (str1[i] != str2[i]) {
            return false;
        }
    }
    
    return true;
}

void T1_std_f32_to_string(
    const f32 input,
    char * recipient,
    const u32 recipient_size)
{
    (void)recipient_size;
    
    f32 temp_above_decimal = (f32)(s32)input;
    s32 below_decimal =
        (s32)((input - temp_above_decimal) * 100000);
    s32 above_decimal = (s32)temp_above_decimal;
    
    T1_std_s32_to_string(
        /* const s32 input: */
            above_decimal,
        /* char * recipient: */
            recipient);
    
    u32 count = 0;
    while (recipient[count] != '\0') {
        count++;
    }
    
    recipient[count] = '.';
    count++;
    
    // count the number of leading 0's after the comma
    f32 mod = 10.0f;
    while (
        (below_decimal > 0) &&
        (u32)((input - temp_above_decimal) * mod) < 1)
    {
        mod *= 10.0f;
        recipient[count++] = '0';
    }
    
    T1_std_s32_to_string(
        /* const s32 input: */
            below_decimal,
        /* char * recipient: */
            recipient + count);
}

void T1_std_s32_to_string(
    const s32 input,
    char * recipient)
{
    if (input < 0) {
        recipient[0] = '-';
        s32 positive_to_append = (input + (input == INT32_MIN)) * -1;
        T1_std_u32_to_string(
            /* input: */ (u32)positive_to_append,
            /* recipient: */ recipient + 1);
    } else {
        T1_std_u32_to_string(
            /* input: */ (u32)input,
            /* recipient: */ recipient);
    }
}

void
T1_std_u32_to_string(
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

s32
T1_std_string_to_int32_validate(
    const char * input,
    u8 * good)
{
    if (input[0] == '\0') {
        *good = false;
        return 0;
    }
    
    // the maximum s32 is 2147483647
    
    if (input[0] == '-') {
        u32 temp = T1_std_string_to_uint32(
            /* input: */
                input + 1);
        
        if (temp > 2147483646) {
            *good = false;
            return 0;
        }
        
        *good = true;
        
        return (s32)temp * -1;
    }
    
    u32 unsigned_return = T1_std_string_to_uint32_validate(input, good);
    
    if (!*good) {
        return 0;
    }
    
    return (s32)unsigned_return;
}

s32
T1_std_string_to_s32(const char * input)
{
    u8 result_good = false;
    s32 result = T1_std_string_to_int32_validate(
        input,
        &result_good);
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    assert(result_good);
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    return result;
}

u32
T1_std_string_to_uint32_validate(
    const char * input,
    u8 * good)
{
    if (input[0] < '0' || input[0] > '9') {
        *good = false;
        return 0;
    } else {
        *good = true;
    }
    
    u32 return_value = 0;
    
    // the maximum u32 is 4294967295
    // so the decimal should   1000000000
    
    u32 decimal = 1;
    s32 i = 0;
    while (input[i] >= '0' && input[i] <= '9') {
        i++;
    }
    i--;
    
    while (i >= 0)
    {
        if (input[i] < '0' || input[i] > '9') {
            return return_value;
        }
        
        u32 current_digit = (u32)(input[i] - '0');
        return_value += decimal * current_digit;
        decimal *= 10;
        
        if (decimal > 1000000000) {
            *good = false;
            return return_value;
        }
        
        i--;
    }
    
    return return_value;
}

u32
T1_std_string_to_uint32(
    const char * input)
{
    u8 result_good = false;
    u32 result = T1_std_string_to_uint32_validate(
        input,
        &result_good);
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    assert(result_good);
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    return result;
}

f32
T1_std_string_to_f32_validate(
    const char * input,
    u8 * good)
{
    if (input[0] == '\0') {
        *good = false;
        return 0;
    }
    
    f32 return_value = 0;
    
    u32 i = 0;
    u8 found_num = false;
    u8 used_dot = false;
    char part1[20];
    u32 part1_size = 0;
    char part2[20];
    u32 part2_size = 0;
    
    u8 found_exponent = false;
    s32 exponent_modifier = 1; 
    s32 exponent = 0;
    
    f32 sign = 1.0f;
    
    if (input[0] == '-') {
        sign = -1.0f;
        i++;
    }
    
    while (
        input[i] != '\0' &&
        input[i] != ' ' &&
        input[i] != '\n' &&
        input[i] != '\r')
    {
        if (!used_dot && found_num && input[i] == '.') {
            i++;
            used_dot = true;
            continue;
        }
        
        if (input[i] >= '0' && input[i] <= '9') {
            found_num = true;
            if (found_exponent) {
                if (exponent == 0) {
                    exponent = input[i] - '0';
                } else {
                    // TODO: exponents of 10 or higher
                    exponent = 10;
                }
            } else if (!used_dot) {
                part1[part1_size++] = input[i];
            } else {
                part2[part2_size++] = input[i];
            }
        } else if (
            input[i] == 'e' || input[i] == 'E')
        {
            if (found_exponent) {
                *good = false;
                return return_value;
            }
            found_exponent = true;
            if (input[i+1] == '-') {
                exponent_modifier = -1;
                i++;
            } else if (input[i+1] == '+') {
                i++;
            } else {
                *good = false;
                return return_value;
            }
        } else {
            *good = false;
            return return_value;
        }
        
        i++;
    }
    
    part1[part1_size] = '\0';
    
    u8 part1_valid = false;
    s32 part1_s32 = T1_std_string_to_int32_validate(
        /* const char input: */ part1,
        &part1_valid);
    if (!part1_valid) {
        *good = false;
        return return_value;
    }
    
    return_value += (f32)part1_s32;
    
    if (part2_size > 0) {
    
        if (part2_size > 6) { part2_size = 6; }
        
        part2[part2_size] = '\0';
        u8 part2_valid = false;
        s32 part2_s32 = T1_std_string_to_int32_validate(
            /* const char input: */ part2,
            &part2_valid);
        if (!part2_valid) {
            *good = false;
            return return_value;
        }
        
        // throw away 0's at the end after the comma, they're useless
        while (part2_s32 % 10 == 0 && part2_size > 0) {
            part2_s32 /= 10;
            part2_size -= 1;
        }
        
        f32 divisor = 1;
        for (u32 _ = 0; _ < part2_size; _++) {
            divisor *= 10;
        }
        return_value += ((f32)part2_s32 / divisor);
    }
    
    return_value *= sign;
    
    // apply the 'scientific notation' exponent
    // e-4 means we want to multiply by -1 * (10^4)
    f32 scinot_modifier = 1;
    for (s32 _ = 0; _ < exponent; _++) {
        scinot_modifier *= (exponent_modifier < 0 ? 0.1f : 10.0f);
    }
    return_value *= scinot_modifier;
    
    *good = true;
    return return_value;
}

f32
T1_std_string_to_f32(
    const char * input)
{
    u8 result_good = false;
    f32 result = T1_std_string_to_f32_validate(
        input,
        &result_good);
    
    #if T1_STD_ASSERTS_ACTIVE == T1_ACTIVE
    assert(result_good);
    #elif T1_STD_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    return result;
}
