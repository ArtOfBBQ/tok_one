#include "T1_common.h"

// void *(*)(void *, int, size_t
void * common_memset_char(
    void * in,
    int value,
    size_t size_bytes)
{
    assert(in != NULL);
    
    int8_t * input = (int8_t *)in;
    
    uint32_t i = 0;
    
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
void common_memset_int16(
    void * in,
    int16_t value,
    unsigned int size_bytes)
{
    int16_t * input = (int16_t *)in;
    
    uint32_t i = 0;
    
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
void common_memset_int32(
    void * in,
    int32_t value,
    unsigned int size_bytes)
{
    int32_t * input = (int32_t *)in;
    
    uint32_t i = 0;
    
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

void common_memset_float(
    void * in,
    float value,
    unsigned int size_bytes)
{
    float * input = (float *)in;
    
    uint32_t i = 0;
    
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

void * common_memcpy(
    void * dest,
    const void * src,
    size_t n_bytes)
{
    uint32_t i = 0;
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

float common_minf(const float x, const float y)
{
    return ((x <= y) * x) + ((y < x) * y);
}

float common_maxf(const float x, const float y)
{
    return ((x >  y) * x) + ((y <= x) * y);
}

int common_mini(const int x, const int y)
{
    return ((x <= y) * x) + ((y < x) * y);
}

int common_maxi(const int x, const int y)
{
    return ((x <= y) * y) + ((x > y) * x);
}

void
common_internal_strcat_capped(
    char * recipient,
    #ifndef COMMON_IGNORE_ASSERTS
    const uint32_t recipient_size,
    #endif
    const char * to_append)
{
    uint32_t i = 0;
    while (recipient[i] != '\0') {
        #ifndef COMMON_IGNORE_ASSERTS
        assert(i < recipient_size);
        #endif
        i++;
    }
    
    uint32_t j = 0;
    while (to_append[j] != '\0') {
        #ifndef COMMON_IGNORE_ASSERTS
        assert(i < recipient_size - 1);
        #endif
        recipient[i++] = to_append[j++];
    }
    
    recipient[i] = '\0';
}

void common_strcat_char_capped(
    char * recipient,
    char to_append)
{
    uint32_t i = 0;
    while (recipient[i] != '\0') {
        i++;
    }
    
    recipient[i++] = to_append;
    
    recipient[i] = '\0';
}

void
common_internal_strcat_int_capped(
    char * recipient,
    #ifndef COMMON_IGNORE_ASSERTS
    const uint32_t recipient_size,
    #endif
    const int32_t to_append)
{
    uint32_t i = 0;
    while (recipient[i] != '\0') {
        #ifndef COMMON_IGNORE_ASSERTS
        assert(i < recipient_size);
        #endif
        i++;
    }
    
    common_int_to_string(to_append, recipient + i);
}

void
common_internal_strcat_uint_capped(
    char * recipient,
    #ifndef COMMON_IGNORE_ASSERTS
    const uint32_t recipient_size,
    #endif
    const uint32_t to_append)
{
    uint32_t i = 0;
    while (recipient[i] != '\0') {
        #ifndef COMMON_IGNORE_ASSERTS
        assert(i < recipient_size);
        #endif
        i++;
    }
    
    common_uint_to_string(to_append, recipient + i);
}

void
common_internal_strcat_float_capped(
    char * recipient,
    #ifndef COMMON_IGNORE_ASSERTS
    const uint32_t recipient_size,
    #endif
    const float to_append)
{
    float positive_append = to_append >= 0.0f ? to_append : -1.0f * to_append;
    
    uint32_t before_comma = (uint32_t)positive_append;
    uint32_t after_comma =
        ((uint32_t)(positive_append * 1000) - (before_comma * 1000));
    if (to_append < 0.0f) {
        common_internal_strcat_capped(
            recipient,
            #ifndef COMMON_IGNORE_ASSERTS
            recipient_size,
            #endif
            "-");
    }
    common_internal_strcat_uint_capped(
        recipient,
        #ifndef COMMON_IGNORE_ASSERTS
        recipient_size,
        #endif
        before_comma);
    common_internal_strcat_capped(
        recipient,
        #ifndef COMMON_IGNORE_ASSERTS
        recipient_size,
        #endif
        ".");
    common_internal_strcat_uint_capped(
        recipient,
        #ifndef COMMON_IGNORE_ASSERTS
        recipient_size,
        #endif
        after_comma);
}

void common_internal_strcpy_capped(
    char * recipient,
    #ifndef COMMON_IGNORE_ASSERTS
    const uint32_t recipient_size,
    #endif
    const char * origin)
{
    uint32_t i = 0;
    while (origin[i] != '\0')
    {
        #ifndef COMMON_IGNORE_ASSERTS
        assert(i < recipient_size - 1);
        #endif
        recipient[i] = origin[i];
        i += 1;
    }
    
    #ifndef COMMON_IGNORE_ASSERTS
    assert(i < recipient_size);
    #endif
    recipient[i] = '\0';
}

void
common_copy_strings(
    char * recipient,
    const uint32_t recipient_size,
    const char * origin,
    const uint32_t origin_size)
{
    uint32_t i = 0;
    for (; i < origin_size; i++) {
        recipient[i] = origin[i];
    }
    
    for (; i < recipient_size; i++) {
        recipient[i] = '\0';
    }
}

uint32_t
common_get_string_length(   
    const char * null_terminated_string)
{
    uint32_t return_value = 0;
    while (
        null_terminated_string[return_value] != '\0')
    {
        return_value++;
    }
    
    return return_value;
}

bool32_t
common_string_starts_with(
    const char * str_to_check,
    const char * start)
{
    if (str_to_check == NULL || start == NULL) {
        return false;
    }
    
    int32_t i = 0;
    
    while (start[i] != '\0') {
        if (str_to_check[i] != start[i]) {
            return false;
        }
        i++;
    }
    
    return true;
}

bool32_t
common_string_ends_with(
    const char * str_to_check,
    const char * ending)
{
    if (str_to_check == NULL || ending == NULL) {
        return false;
    }
    
    uint32_t str_to_check_len = common_get_string_length(str_to_check);
    uint32_t ending_len = common_get_string_length(ending);
    
    if (ending_len > str_to_check_len || ending_len < 1) {
        return false;
    }
    
    uint32_t i = str_to_check_len;
    uint32_t j = ending_len;
    
    while (j > 0) {
        j--;
        i--;
        
        if (str_to_check[i] != ending[j]) {
            return false;
        }
    }
    
    return true;
}

bool32_t
common_are_equal_strings(
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
    
    uint32_t i = 0;
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

bool32_t
common_are_equal_until_nullterminator(
    const char * str1,
    const char * str2)
{
    if (str1 == NULL || str2 == NULL) {
        return false;
    }
    
    uint32_t i = 0;
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

bool32_t
common_are_equal_strings_of_length(
    const char * str1,
    const char * str2,
    const uint64_t len)
{
    for (uint64_t i = 0; i < len; i++) {
        if (str1[i] != str2[i]) {
            return false;
        }
    }
    
    return true;
}

void common_float_to_string(
    const float input,
    char * recipient,
    const uint32_t recipient_size)
{
    (void)recipient_size;
    
    float temp_above_decimal = (float)(int32_t)input;
    int32_t below_decimal =
        (int32_t)((input - temp_above_decimal) * 100000);
    int32_t above_decimal = (int32_t)temp_above_decimal;
    
    common_int_to_string(
        /* const int32_t input: */
            above_decimal,
        /* char * recipient: */
            recipient);
    
    uint32_t count = 0;
    while (recipient[count] != '\0') {
        count++;
    }
    
    recipient[count] = '.';
    count++;
    
    // count the number of leading 0's after the comma
    float mod = 10.0f;
    while (
        (below_decimal > 0) &&
        (uint32_t)((input - temp_above_decimal) * mod) < 1)
    {
        mod *= 10.0f;
        recipient[count++] = '0';
    }
    
    common_int_to_string(
        /* const int32_t input: */
            below_decimal,
        /* char * recipient: */
            recipient + count);
}

void common_int_to_string(
    const int32_t input,
    char * recipient)
{
    if (input < 0) {
        recipient[0] = '-';
        int32_t positive_to_append = (input + (input == INT32_MIN)) * -1;
        common_uint_to_string(
            /* input: */ (uint32_t)positive_to_append,
            /* recipient: */ recipient + 1);
    } else {
        common_uint_to_string(
            /* input: */ (uint32_t)input,
            /* recipient: */ recipient);
    }
}

void
common_uint_to_string(
    const uint32_t input,
    char * recipient)
{
    if (input == 0) {
        recipient[0] = '0';
        recipient[1] = '\0';
        return;
    }
    
    uint32_t i = 0;
    uint32_t start_i = 0;
    uint32_t end_i;
    
    uint64_t decimal = 1;
    uint32_t input_div_dec = input;
    while (input_div_dec > 0) {
        uint32_t isolated_num = input % (decimal * 10);
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

int32_t
common_string_to_int32_validate(
    const char * input,
    bool32_t * good)
{
    if (input[0] == '\0') {
        *good = false;
        return 0;
    }
    
    // the maximum int32_t is 2147483647
    
    if (input[0] == '-') {
        uint32_t temp = common_string_to_uint32(
            /* input: */
                input + 1);
        
        if (temp > 2147483646) {
            *good = false;
            return 0;
        }
        
        *good = true;
        
        return (int32_t)temp * -1;
    }
    
    uint32_t unsigned_return = common_string_to_uint32_validate(input, good);
    
    if (!*good) {
        return 0;
    }
    
    return (int32_t)unsigned_return;
}

int32_t
common_string_to_int32(const char * input)
{
    bool32_t result_good = false;
    int32_t result = common_string_to_int32_validate(
        input,
        &result_good);
    #ifndef COMMON_IGNORE_ASSERTS
    assert(result_good);
    #endif
    return result;
}

uint32_t
common_string_to_uint32_validate(
    const char * input,
    bool32_t * good)
{
    if (input[0] < '0' || input[0] > '9') {
        *good = false;
        return 0;
    } else {
        *good = true;
    }
    
    uint32_t return_value = 0;
    
    // the maximum uint32_t is 4294967295
    // so the decimal should   1000000000
    
    uint32_t decimal = 1;
    int32_t i = 0;
    while (input[i] >= '0' && input[i] <= '9') {
        i++;
    }
    i--;
    
    while (i >= 0)
    {
        if (input[i] < '0' || input[i] > '9') {
            return return_value;
        }
        
        uint32_t current_digit = (uint32_t)(input[i] - '0');
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

uint32_t
common_string_to_uint32(
    const char * input)
{
    bool32_t result_good = false;
    uint32_t result = common_string_to_uint32_validate(
        input,
        &result_good);
    #ifndef COMMON_IGNORE_ASSERTS
    assert(result_good);
    #endif
    return result;
}

float
common_string_to_float_validate(
    const char * input,
    bool32_t * good)
{
    if (input[0] == '\0') {
        *good = false;
        return 0;
    }
    
    float return_value = 0;
    
    uint32_t i = 0;
    bool32_t found_num = false;
    bool32_t used_dot = false;
    char first_part[20];
    uint32_t first_part_size = 0;
    char second_part[20];
    uint32_t second_part_size = 0;
   
    bool32_t found_exponent = false;
    int32_t exponent_modifier = 1; 
    int32_t exponent = 0;
    
    if (input[0] == '-') {
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
                first_part[first_part_size++] = input[i];
            } else {
                second_part[second_part_size++] = input[i];
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
    
    first_part[first_part_size] = '\0';
    
    bool32_t first_part_valid = false;
    int first_part_int = common_string_to_int32_validate(
        /* const char input: */ first_part,
        &first_part_valid);
    if (!first_part_valid) {
        *good = false;
        return return_value;
    }

    if (second_part_size > 0) {
    
        if (second_part_size > 6) { second_part_size = 6; }
        
        second_part[second_part_size] = '\0';
        bool32_t second_part_valid = false;
        int second_part_int = common_string_to_int32_validate(
            /* const char input: */ second_part,
            &second_part_valid);
        if (!second_part_valid) {
            *good = false;
            return return_value;
        }
        
        // throw away 0's at the end after the comma, they're useless
        while (second_part_int % 10 == 0 && second_part_size > 0) {
            second_part_int /= 10;
            second_part_size -= 1;
        }
	
        return_value += (float)first_part_int;
        float divisor = 1;
        for (uint32_t _ = 0; _ < second_part_size; _++) {
            divisor *= 10;
        }
        return_value += ((float)second_part_int / divisor);
        
        if (input[0] == '-') {
            return_value *= -1;
        }
    }
    
    // apply the 'scientific notation' exponent
    // e-4 means we want to multiply by -1 * (10^4)
    float scinot_modifier = 1;
    for (int32_t _ = 0; _ < exponent; _++) {
        scinot_modifier *= (exponent_modifier < 0 ? 0.1f : 10.0f);
    }
    return_value *= scinot_modifier;
    
    *good = true;
    return return_value;
}

float
common_string_to_float(
    const char * input)
{
    bool32_t result_good = false;
    float result = common_string_to_float_validate(
        input,
        &result_good);
    
    #ifndef COMMON_IGNORE_ASSERTS
    assert(result_good);
    #endif
    return result;
}
