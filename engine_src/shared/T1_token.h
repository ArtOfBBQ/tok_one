#ifndef T1_TOKEN_H
#define T1_TOKEN_H

#include "T1_public_types.h"

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

void
T1_token_set_store_mode(T1TokenStoreMode mode);

void
T1_token_set_reg_bitflags(u8 bitflags);

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
    u32 pattern_index);

void
T1_token_set_reg_middle_cap(u32 middle_cap);

void T1_token_set_string_literal(
    u32 enum_value,
    u8 * good);

void
T1_token_register(
    u32 enum_value,
    u8 * good);

/*
After setting everything up, run this function to actually do the work of
transforming text into the tokens you specified
*/
void
T1_token_run(
    const char * input,
    u8 * good);

u32 T1_token_get_token_count(void);
u32 T1_token_get_enum_value(u16 token_i);
void T1_token_overwrite_enum_val(u16 token_i, u32 new_val);
char * T1_token_get_string_value(u16 token_i);
u32 T1_token_get_string_value_size(u16 token_i);
u32 T1_token_get_line_num(u16 token_i);

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
b8 T1_token_is_number(s32 at_i);
b8 T1_token_fits_f64(s32 at_i);
b8 T1_token_fits_f32(s32 at_i);
b8 T1_token_fits_s64(s32 at_i);
b8 T1_token_fits_s32(s32 at_i);
b8 T1_token_fits_s16(s32 at_i);
b8 T1_token_fits_s8(s32 at_i);
b8 T1_token_fits_u64(s32 at_i);
b8 T1_token_fits_u32(s32 at_i);
b8 T1_token_fits_u16(s32 at_i);
b8 T1_token_fits_u8(s32 at_i);

u64 T1_token_as_number_unsigned(s32 at_i);
s64 T1_token_as_number_signed(s32 at_i);
f64 T1_token_as_number_floating(s32 at_i);

#endif // T1_TOKEN_H
