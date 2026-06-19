#ifndef T1_TOKEN_H
#define T1_TOKEN_H

#include "T1_stdint.h"


/*
THE API FOR TRANSFORMING TEXT TO TOKENS (below)
*/

/*
The tokenizer uses memory, so you need to initialize it (with malloc or your
own malloc function) before doing anything else.
*/
void
T1_token_init(
    void * (* arg_memset_func)(void *, int, u64),
    u64 (* arg_strlen_func)(const char *),
    void * (* arg_malloc_func)(u64),
    u8 * good);

void
T1_token_deinit(
    void (* arg_free_func)(void *));

/*
If you want to tokenize multiple separate file formats in your program, call the
reset() function in between each run to clear all registered tokens.

You don't need to do this the 1st time, the init() also does a reset().
*/
void
T1_token_reset(u8 * good);

/*
Before running the tokenizer, register your enums with some ascii values that
are convenient for you.
*/

#define T1_TOKEN_FLAG_IGNORE_CASE 1
#define T1_TOKEN_FLAG_SCIENTIFIC_OK 2
#define T1_TOKEN_FLAG_LEAD_DOT_OK 4
#define T1_TOKEN_FLAG_PRECISE 8
#define T1_TOKEN_FLAG_CONSUME_STOP_PATTERN 32

typedef enum : u8 {
    T1_TOKEN_STOREMODE_DISCARD_TOKEN,    // Don't even register the token
    T1_TOKEN_STOREMODE_DISCARD_STRING,   // Register token, discard string
    T1_TOKEN_STOREMODE_FULLSTARTMIDSTOP, // Register token, save string
    T1_TOKEN_STOREMODE_MIDDLE_STRING     // Register token, save string
} T1TokenStoreMode;

void
T1_token_set_store_mode(const T1TokenStoreMode mode);

void
T1_token_set_reg_bitflags(
    const u8 bitflags);

void
T1_token_clear_start_pattern(void);

void
T1_token_set_reg_start_pattern(
    const char * start_pattern);

void
T1_token_clear_stop_patterns(void);

void
T1_token_set_reg_stop_pattern(
    const char * stop_pattern,
    const u32 pattern_index);

void
T1_token_set_reg_middle_cap(
    const u32 middle_cap);

void
T1_token_set_string_literal(
    const u32 enum_value,
    u8 * good);

void
T1_token_register(
    const u32 enum_value,
    u8 * good);

/*
After setting everything up, run this function to actually do the work of
transforming text into the tokens you specified
*/
void
T1_token_run(
    const char * input,
    u8 * good);


/*
THE API FOR QUERYING TOKEN DATA (below)

After you've succesfully created tokens, you can fetch their data with these
functions.

The Token's "value" points to internal memory and will be erased when you
reset(), so use the data immediately or copy it if you need it permanently.

If you need to know if a token representing a string literal is a number, or
if it could be cast to a u16 without losing data, use the castable flags
directly or use these convenience macros. For example:
if (T1_token_is_u16(token)) {
    // your logic...
}

the T1_token_is_number(T1Token*) macro will return 0 if a number is too
big for a uint64, even if it's composed of all numbers
*/
typedef struct {
    u64 as_u64;
    s64 as_i64;
    f64 as_f64;
} T1TokenNumber;

#define T1_token_is_number(tokenptr) ((tokenptr)->castable_flags & 1)
#define T1_token_fits_f64(tokenptr) (((tokenptr)->castable_flags & 2) > 0)
#define T1_token_fits_f32(tokenptr) (((tokenptr)->castable_flags & 4) > 0)
#define T1_token_fits_u8(tokenptr) (((tokenptr)->castable_flags & 8) > 0)
#define T1_token_fits_u16(tokenptr) (((tokenptr)->castable_flags & 16) > 0)
#define T1_token_fits_u32(tokenptr) (((tokenptr)->castable_flags & 32) > 0)
#define T1_token_fits_u64(tokenptr) (((tokenptr)->castable_flags & 64) > 0)
#define T1_token_fits_i8(tokenptr) (((tokenptr)->castable_flags & 128) > 0)
#define T1_token_fits_i16(tokenptr) (((tokenptr)->castable_flags & 256) > 0)
#define T1_token_fits_s32(tokenptr) (((tokenptr)->castable_flags & 512) > 0)
#define T1_token_fits_i64(tokenptr) (((tokenptr)->castable_flags & 1024) > 0)
typedef struct {
    T1TokenNumber * number_value;
    u32 enum_value;
    u32 line_number;
    char * string_value;
    u16 string_value_size; // size in bytes
    u16 castable_flags;
} T1Token;

u32
T1_token_get_token_count(void);

T1Token *
T1_token_get_token_at(
    const u32 token_i);

#endif // T1_TOKEN_H
