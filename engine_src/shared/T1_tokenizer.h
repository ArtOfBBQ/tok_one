#ifndef T1_TOKEN_H
#define T1_TOKEN_H

#include <stddef.h>
#include <stdint.h>

#ifndef T1_TOKEN_NO_ASSERTS
#include <assert.h>
#endif


/*
THE API FOR TRANSFORMING TEXT TO TOKENS (below)
*/

/*
The tokenizer uses memory, so you need to initialize it (with malloc or your
own malloc function) before doing anything else.
*/
void T1_token_init(
    void * (* arg_memset_func)(void *, int, size_t),
    size_t (* arg_strlen_func)(const char *),
    void * (* arg_malloc_func)(size_t),
    uint32_t * good);

void T1_token_deinit(
    void (* arg_free_func)(void *));

/*
If you want to tokenize multiple separate file formats in your program, call the
reset() function in between each run to clear all registered tokens.

You don't need to do this the 1st time, the init() also does a reset().
*/
void T1_token_reset(uint32_t * good);

/*
Before running the tokenizer, register your enums with some ascii values that
are convenient for you.
*/

#define T1_TOKEN_FLAG_IGNORE_CASE 1
#define T1_TOKEN_FLAG_SCIENTIFIC_OK 2
#define T1_TOKEN_FLAG_LEAD_DOT_OK 4
#define T1_TOKEN_FLAG_PRECISE 8
#define T1_TOKEN_FLAG_CONSUME_STOP_PATTERN 32

typedef enum : uint8_t {
    T1_TOKEN_STOREMODE_DISCARD_TOKEN,    // Don't even register the token
    T1_TOKEN_STOREMODE_DISCARD_STRING,   // Register token, discard string
    T1_TOKEN_STOREMODE_FULLSTARTMIDSTOP, // Register token, save string
    T1_TOKEN_STOREMODE_MIDDLE_STRING // Register token, save string
} T1TokenStoreMode;

void T1_token_set_store_mode(const T1TokenStoreMode mode);
void T1_token_set_reg_bitflags(
    const uint8_t bitflags);
void T1_token_clear_start_pattern(void);
void T1_token_set_reg_start_pattern(
    const char * start_pattern);
void T1_token_clear_stop_patterns(void);
void T1_token_set_reg_stop_pattern(
    const char * stop_pattern,
    const uint32_t pattern_index);
void T1_token_set_reg_middle_cap(
    const uint32_t middle_cap);

void T1_token_set_string_literal(
    const uint32_t enum_value,
    uint32_t * good);
void T1_token_register(
    const uint32_t enum_value,
    uint32_t * good);

/*
After setting everything up, run this function to actually do the work of
transforming text into the tokens you specified
*/
void T1_token_run(
    const char * input,
    uint32_t * good);


/*
THE API FOR QUERYING TOKEN DATA (below)

After you've succesfully created tokens, you can fetch their data with these
functions.

The Token's "value" points to internal memory and will be erased when you
reset(), so use the data immediately or copy it if you need it permanently.

If you need to know if a token representing a string literal is a number, or
if it could be cast to a u16 without losing data, use the castable flags
directly or use these convenience macros. For example:
if (toktoken_is_u16(token)) {
    // your logic...
}

the toktoken_is_number(TokToken*) macro will return 0 if a number is too
big for a uint64, even if it's composed of all numbers
*/
typedef struct TokTokenNumber {
    uint64_t unsigned_int;
    int64_t signed_int;
    double double_precision;
} TokTokenNumber;
#define toktoken_is_number(tokenptr) ((tokenptr)->castable_flags & 1)
#define toktoken_fits_double(tokenptr) (((tokenptr)->castable_flags & 2) > 0)
#define toktoken_fits_float(tokenptr) (((tokenptr)->castable_flags & 4) > 0)
#define toktoken_fits_u8(tokenptr) (((tokenptr)->castable_flags & 8) > 0)
#define toktoken_fits_u16(tokenptr) (((tokenptr)->castable_flags & 16) > 0)
#define toktoken_fits_u32(tokenptr) (((tokenptr)->castable_flags & 32) > 0)
#define toktoken_fits_u64(tokenptr) (((tokenptr)->castable_flags & 64) > 0)
#define toktoken_fits_i8(tokenptr) (((tokenptr)->castable_flags & 128) > 0)
#define toktoken_fits_i16(tokenptr) (((tokenptr)->castable_flags & 256) > 0)
#define toktoken_fits_i32(tokenptr) (((tokenptr)->castable_flags & 512) > 0)
#define toktoken_fits_i64(tokenptr) (((tokenptr)->castable_flags & 1024) > 0)
typedef struct TokToken {
    TokTokenNumber * number_value;
    uint32_t enum_value;
    uint32_t line_number;
    char * string_value;
    uint16_t string_value_size; // size in bytes
    uint16_t castable_flags;
} TokToken;

uint32_t T1_token_get_token_count(void);
TokToken * T1_token_get_token_at(const uint32_t token_i);


#endif // T1_TOKEN_H
