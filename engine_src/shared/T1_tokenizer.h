#ifndef TOKTOKEN_H
#define TOKTOKEN_H

#include <stddef.h>
#include <stdint.h>

/*
WHAT IS THIS FILE?

Use this tokenizer to pass a text file with ASCII inputs and get back a list
of tokens. You register your enum values and the pattern they need to match.

For example, you could create a string:
"if 1 + 5 = stringliteral"

..and define an enum (what you find convenient):
typedef enum MyEnum {
    TOKEN_IF,
    TOKEN_PLUS,
    TOKEN_NUMBER,
    ...
} MyEnum;

...and get back a list of tokens:
TOKEN_IF
TOKEN_NUMBER (value 1)
TOKEN_PLUS
TOKEN_NUMBER (value 5)
TOKEN_EQUALS
TOKEN_STRING_LITERAL (value 'stringliteral')
*/




/*
HOW TO USE THIS FILE?

Instead of reading documentation, you can learn with THE DEBUG PRINT FUNCTION
(below) or the EXAMPLE PROGRAM (end of this file).

This tokenizer is 'eager', so when tokens are returned they will already
have numerical data converted. The work is wasted if you didn't need to do
such conversions.

None of these functions are thread-safe, I'm assuming you want to tokenize on
1 thread only.

Many functions take a "uint32_t * good" argument, this is set to 1 for
success and 0 for failure. You don't need to initialize 'good' before
passing it, it is guaranteed to be set. Once a function is not 'good', all
other functions will also fail until you call toktoken_reset().
*/




/*
THE DEBUG PRINT FUNCTION

This function is not necessary for operation, it's just here to help you learn
and debug. If you call it and pass printf, it will print a detailed
explanation of the tokenizer's current state, so you can understand exactly
where you are, what went wrong (if anything), etc. It's only available in
debug builds.

Instead of reading documentation, you could learn with this 6 line program:

main.c:

#include <stdio.h>
#include "toktokenizer.h"

int main(void) {
    toktoken_debug_print_state(printf);
}

Compile this (in debug mode so NDEBUG is not set), run, read the instructions,
and fix a thing. Repeat until you're succesfully tokenizing.
*/
#ifndef NDEBUG
void toktoken_debug_print_state(
    int (*arg_printf)(const char *, ... ));
#endif




/*
THE API FOR TRANSFORMING TEXT TO TOKENS (below)
*/

/*
The tokenizer uses memory, so you need to initialize it (with malloc or your
own malloc function) before doing anything else.
*/
void toktoken_init(
    void * (*arg_memset_func)(void *, int, size_t),
    void * (*arg_malloc_func)(size_t),
    uint32_t * good);

/*
If you want to tokenize multiple separate things in your program, call the
reset() function in between each run.

You don't need to do this the 1st time, the init() also does a reset().
*/
void toktoken_reset(uint32_t * good);

/*
Before running the tokenizer, register your enums with some ascii values that
are convenient for you.

The 'string literal' is used as a last resort, for strings that don't match
any other tokens you've specified.
*/
void toktoken_register_newline_enum(
    const uint32_t enum_value,
    uint32_t * good);
void toktoken_register_string_literal_enum(
    const uint32_t enum_value,
    uint32_t * good);
void toktoken_register_token(
    const char * ascii_value,
    const uint32_t enum_value,
    uint32_t * good);

/*
Adjust next_registration_settings' fields before registering tokens to modify
behavior, this struct is exposed to you, and the values are essentially
optional arguments for the toktoken_register_token() function.
*/
typedef struct TokTokenClientSettings {
    uint8_t next_token_ignore_case;
    uint8_t next_token_ignore_whitespace;
    uint8_t allow_scientific_notation; // 1 = allow "1.23e-4", 0 = reject
    uint8_t allow_leading_dot;         // 1 = allow ".123", 0 = reject
    uint8_t allow_high_precision;      // 1 = allow >16 digits with rounding
} TokTokenClientSettings;
extern TokTokenClientSettings * toktoken_client_settings;

/*
After setting everything up, run this function to actually do the work of
transforming text into the tokens you specified
*/
void toktoken_run(const char * input, uint32_t * good);


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

uint32_t toktoken_get_token_count(void);
TokToken * toktoken_get_token_at(const uint32_t token_i);



/*
EXAMPLE PROGRAM (try compiling and running this)

main.c:

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include "toktokenizer.h"

typedef enum SampleEnum {
    SAMPLEENUM_BLA,
    SAMPLEENUM_DARK_SIDE,
    SAMPLEENUM_STRING_LITERAL,
} SampleEnum;

int main(void) {
    uint32_t good;
    toktoken_init(malloc, &good);
    assert(good);
    
    toktoken_register_string_literal_enum(SAMPLEENUM_STRING_LITERAL, &good);
    
    toktoken_register_token("bla", SAMPLEENUM_BLA, &good);
    toktoken_register_token("dark side", SAMPLEENUM_DARK_SIDE, &good);
    assert(good);
    
    toktoken_run(
        "bla bla bla dark side bla imaliteral dark side",
        &good);
    assert(good);
    
    uint32_t tokens_count = toktoken_get_token_count();
    for (uint32_t i = 0; i < tokens_count; i++) {
        TokToken * token = toktoken_get_token_at(i);
        printf(
            "token %u: %s (enum value: %u)\n",
            i,
            token->value,
            token->enum_value);
    }
}
*/

#endif // TOKTOKEN_H

